#define NOMINMAX
#include "global.h"
#include "utils.h"
#include "nodedefs.h"
#include "rgtree.h"
#include <numeric>

using namespace slkc;
using namespace slkc::ast;

SLKC_API NodeRegistry::~NodeRegistry() {
}

SLKC_API GreenNodeRegistry::~GreenNodeRegistry() {
}

SLKC_API GlobalSharedString::GlobalSharedString() noexcept {
}

SLKC_API GlobalSharedString::~GlobalSharedString() {
	if (_ptr)
		_global->get_allocator()->release(_ptr, _length, alignof(char));
}

SLKC_API void Global::_add_ast_node_to_deferred_deleting_list(AstNode *node) noexcept {
	node->_next_destructible = _zero_ref_ast_node_registry_list;
	_zero_ref_ast_node_registry_list = node;
}

SLKC_API void Global::_add_green_node_to_deferred_deleting_list(GreenNode *green_node) noexcept {
	green_node->_next_destructible = _zero_ref_green_node_registry_list;
	_zero_ref_green_node_registry_list = green_node;
}

SLKC_API Global::Global(peff::Alloc *allocator) noexcept
	: resource_allocator(allocator),
	  _ast_node_registries(allocator),
	  _green_node_registries(allocator),
	  _shared_strings(allocator) {
}

SLKC_API Global::~Global() noexcept {
	_clear_zero_ref_ast_node_registry_list();
	_clear_zero_ref_green_node_registry_list();
	if (_ast_node_registries.size())
		std::terminate();
	if (_green_node_registries.size())
		std::terminate();
	if (_shared_strings.size())
		std::terminate();
}

SLKC_API void Global::_clear_zero_ref_ast_node_registry_list() noexcept {
	while (_zero_ref_ast_node_registry_list) {
		AstNode *i = _zero_ref_ast_node_registry_list;
		_zero_ref_ast_node_registry_list = nullptr;
		for (AstNode *next = nullptr; i; i = next) {
			std::lock_guard g(_ast_node_registries_mutex);
			next = i->_next_destructible;
			if (i->_node_index < _min_free_ast_node_index)
				_min_free_ast_node_index = i->_node_index;
			i->dealloc();
		}
	}
}

SLKC_API void Global::_clear_zero_ref_green_node_registry_list() noexcept {
	while (_zero_ref_green_node_registry_list) {
		GreenNode *i = _zero_ref_green_node_registry_list;
		_zero_ref_green_node_registry_list = nullptr;
		for (GreenNode *next = nullptr; i; i = next) {
			std::lock_guard g(_green_node_registries_mutex);
			next = i->_next_destructible;
			if (i->_node_index < _min_free_green_node_index)
				_min_free_green_node_index = i->_node_index;
			peff::destroy_and_release<GreenNode>(get_allocator(), i, alignof(GreenNode));
		}
	}
}

SLKC_API void Global::unref_ast_node(AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.pin_count)) {
		_add_ast_node_to_deferred_deleting_list(reg.in_memory);
		_ast_node_registries.remove(index);
	}
}

SLKC_API peff::Result<AstNode *, PinFailReason> Global::pin_ast_node(AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);

	++reg.pin_count;

	return +reg.in_memory;
}

SLKC_API void Global::unpin_ast_node(AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);
	if ((!--reg.pin_count) && (!reg.ref_count)) {
		_add_ast_node_to_deferred_deleting_list(reg.in_memory);
		_ast_node_registries.remove(index);
	}
}

SLKC_API AstNodeIndex Global::_alloc_ast_node_index() noexcept {
	{
		AstNodeIndex i = _min_free_ast_node_index;
		while (i < std::numeric_limits<AstNodeIndex>::max()) {
			if (!_ast_node_registries.contains(i)) {
				++_min_free_ast_node_index;
				return i;
			}
			if (i < std::numeric_limits<AstNodeIndex>::max() / 2) {
				auto it = _ast_node_registries.find_max_lteq(std::numeric_limits<AstNodeIndex>::max() - i);

				if (it != _ast_node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return INVALID_AST_NODE_INDEX;
}

SLKC_API peff::Option<AstNodeIndex> Global::map_ast_node(AstNode *node, AstNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	if (node_index == INVALID_AST_NODE_INDEX) {
		if ((node_index = _alloc_ast_node_index()) == INVALID_AST_NODE_INDEX)
			return INVALID_AST_NODE_INDEX;
	}

	assert(!this->_ast_node_registries.contains(node_index));

	NodeRegistry reg;

	reg.in_memory = node;

	reg.self_index = node_index;

	if (!this->_ast_node_registries.insert(+node_index, std::move(reg)))
		return peff::NULLOPT;

	node->set_node_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_ast_node(AstNodeIndex node_index, AstNode *node) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	assert(this->_ast_node_registries.contains(node_index));

	this->_ast_node_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_ast_node(AstNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	this->_ast_node_registries.remove(node_index);
}

SLKC_API void Global::unref_green_node(AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.pin_count)) {
		_add_green_node_to_deferred_deleting_list(reg.in_memory);
		_green_node_registries.remove(index);
	}
}

SLKC_API peff::Result<GreenNode *, PinFailReason> Global::pin_green_node(GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);

	++reg.pin_count;

	// TODO: Use actual process instead of this.
	return +reg.in_memory;
}

SLKC_API void Global::unpin_green_node(GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);
	if ((!--reg.pin_count) && (!reg.ref_count)) {
		_add_green_node_to_deferred_deleting_list(reg.in_memory);
		_green_node_registries.remove(index);
	}
}

SLKC_API GreenNodeIndex Global::_alloc_green_node_index() noexcept {
	{
		GreenNodeIndex i = _min_free_green_node_index;
		while (i < std::numeric_limits<AstNodeIndex>::max()) {
			if (!_green_node_registries.contains(i)) {
				++_min_free_green_node_index;
				return i;
			}
			if (i < std::numeric_limits<AstNodeIndex>::max() / 2) {
				auto it = _green_node_registries.find_max_lteq(std::numeric_limits<AstNodeIndex>::max() - i);

				if (it != _green_node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return INVALID_AST_NODE_INDEX;
}

SLKC_API peff::Option<GreenNodeIndex> Global::map_green_node(GreenNode *node, GreenNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	if (node_index == INVALID_GREEN_NODE_INDEX) {
		if ((node_index = _alloc_green_node_index()) == INVALID_GREEN_NODE_INDEX)
			return INVALID_GREEN_NODE_INDEX;
	}

	assert(!this->_green_node_registries.contains(node_index));

	GreenNodeRegistry reg;

	reg.in_memory = node;

	reg.self_index = node_index;

	if (!this->_green_node_registries.insert(+node_index, std::move(reg)))
		return peff::NULLOPT;

	node->set_node_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_green_node(GreenNodeIndex node_index, GreenNode *node) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	assert(this->_green_node_registries.contains(node_index));

	this->_green_node_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_green_node(GreenNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	this->_green_node_registries.remove(node_index);
}

SLKC_API peff::Result<AstNodeIndex, DuplicationError> Global::duplicate_ast_node(AstNodeIndex node_index) noexcept {
	AstNodeDuplicationContext context(this);

	AstNodePtr<AstNode> node(this, node_index);
	AstNodePin<AstNode> pinned = node.pin();

	AstNodeIndex new_index;
	{
		auto map_result = this->map_ast_node(nullptr);
		if (!map_result.has_value())
			return DuplicationError::OutOfMemory;
		if (map_result.value() == INVALID_AST_NODE_INDEX)
			return DuplicationError::NoSlot;
		new_index = map_result.value();
	}

	auto result = pinned->do_duplicate(context, new_index);

	if (result.has_error())
		return std::move(result).error();

	this->remap_ast_node(new_index, result.value());

	peff::ScopeGuard unmap_guard([this, new_index]() noexcept {
		this->unmap_ast_node(new_index);
	});

	while (!context.task_list.size()) {
		auto old_task_list = std::move(context.task_list);

		context.task_list = { resource_allocator.get() };

		while (old_task_list.size()) {
			auto task = old_task_list.front();

			AstNodePtr<AstNode> dup_node(this, task.src);
			AstNodePin<AstNode> dup_pinned = dup_node.pin();

			auto dup_result = dup_pinned->do_duplicate(context, new_index);

			if (dup_result.has_error())
				return std::move(dup_result).error();

			this->remap_ast_node(task.dest, dup_result.value());

			old_task_list.pop_front();
		}
	}

	unmap_guard.release();

	return new_index;
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::shallow_dump_ast_node(peff::Alloc *allocator, AstNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	AstNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			AstNodePtr<AstNode> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::deep_dump_ast_node(peff::Alloc *allocator, AstNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	AstNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			AstNodePtr<AstNode> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, true));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::shallow_dump_green_node(peff::Alloc *allocator, GreenNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	GreenNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			GreenNodePtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_green_node(dump_context, i.dest, pinned_src, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::deep_dump_green_node(peff::Alloc *allocator, GreenNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	GreenNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			GreenNodePtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_green_node(dump_context, i.dest, pinned_src, true));
		}
	}

	return root_value.release();
}

SLKC_API GlobalSharedString *Global::register_shared_string(std::string_view sv) noexcept {
	std::lock_guard g(_shared_strings_mutex);
	if (auto it = _shared_strings.find_alt(sv); it != _shared_strings.end())
		return &*it;
	char *s = static_cast<char *>(resource_allocator->alloc(sv.size(), alignof(char)));

	if (!s)
		return nullptr;

	peff::ScopeGuard sg([this, s, &sv]() noexcept {
		resource_allocator->release(s, sv.size(), alignof(char));
	});

	memcpy(s, sv.data(), sv.size());

	GlobalSharedString ss;

	ss._global = this;
	ss._length = sv.size();
	ss._ptr = s;
	ss._ref_count = 0;

	if (!_shared_strings.insert(std::move(ss)))
		return nullptr;

	sg.release();

	return &_shared_strings.at_alt(sv);
}

SLKC_API void Global::unregister_shared_string(std::string_view s) noexcept {
	std::lock_guard g(_shared_strings_mutex);
	_shared_strings.remove_alt(s);
}

SLKC_API bool Global::init_root_module() noexcept {
	if (_root_module != INVALID_AST_NODE_INDEX)
		return true;

	auto raw_node_ptr = make_ast_node<ModuleNode>(this);
	if (!raw_node_ptr)
		return false;

	_root_module = raw_node_ptr->get_node_index();

	return true;
}
