#define NOMINMAX
#include "global.h"
#include "ast/rgtree.h"
#include "ast/astnode.h"
#include "comp/type.h"
#include "comp/env.h"
#include <numeric>

using namespace slkc;

SLKC_API AstNodeRegistry::~AstNodeRegistry() {
}

SLKC_API GreenNodeRegistry::~GreenNodeRegistry() {
}

SLKC_API TypeDefRegistry::~TypeDefRegistry() {
}

SLKC_API GlobalSharedString::GlobalSharedString() noexcept {
}

SLKC_API GlobalSharedString::~GlobalSharedString() {
	if (_ptr)
		_global->get_allocator()->release(_ptr, _length, alignof(char));
}

SLKC_API void Global::_add_ast_node_to_deferred_deleting_list(ast::AstNode *node) noexcept {
	node->_next_destructible = _zero_ref_ast_node_registry_list;
	_zero_ref_ast_node_registry_list = node;
}

SLKC_API void Global::_add_green_node_to_deferred_deleting_list(ast::GreenNode *green_node) noexcept {
	green_node->_next_destructible = _zero_ref_green_node_registry_list;
	_zero_ref_green_node_registry_list = green_node;
}

SLKC_API void Global::_add_type_def_to_deferred_deleting_list(comp::TypeDef* type_def) noexcept {

}

SLKC_API Global::Global(peff::Alloc *allocator) noexcept
	: resource_allocator(allocator),
	  _ast_node_registries(allocator),
	  _green_node_registries(allocator),
	  _type_def_registries(allocator),
	  _shared_strings(allocator) {
}

SLKC_API Global::~Global() noexcept {
	_clear_zero_ref_type_def_registry_list();
	_clear_zero_ref_ast_node_registry_list();
	_clear_zero_ref_green_node_registry_list();
	if (_type_def_registries.size())
		std::terminate();
	if (_ast_node_registries.size())
		std::terminate();
	if (_green_node_registries.size())
		std::terminate();
	if (_shared_strings.size())
		std::terminate();
}

SLKC_API void Global::_clear_zero_ref_ast_node_registry_list() noexcept {
	while (_zero_ref_ast_node_registry_list) {
		ast::AstNode *i = _zero_ref_ast_node_registry_list;
		_zero_ref_ast_node_registry_list = nullptr;
		for (ast::AstNode *next = nullptr; i; i = next) {
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
		ast::GreenNode *i = _zero_ref_green_node_registry_list;
		_zero_ref_green_node_registry_list = nullptr;
		for (ast::GreenNode *next = nullptr; i; i = next) {
			std::lock_guard g(_green_node_registries_mutex);
			next = i->_next_destructible;
			if (i->_node_index < _min_free_green_node_index)
				_min_free_green_node_index = i->_node_index;
			peff::destroy_and_release<ast::GreenNode>(get_allocator(), i, alignof(ast::GreenNode));
		}
	}
}

SLKC_API void Global::_clear_zero_ref_type_def_registry_list() noexcept {
	while (_zero_ref_type_def_registry_list) {
		comp::TypeDef *i = _zero_ref_type_def_registry_list;
		_zero_ref_type_def_registry_list = nullptr;
		for (comp::TypeDef *next = nullptr; i; i = next) {
			std::lock_guard g(_type_def_registries_mutex);
			next = i->_next_destructible;
			if (i->_type_def_index < _min_free_type_def_index)
				_min_free_type_def_index = i->_type_def_index;
			i->dealloc();
		}
	}
}

SLKC_API bool Global::try_ref_ast_node(ast::AstNodeIndex index) noexcept {
	_clear_zero_ref_ast_node_registry_list();
	auto &ref_count = _ast_node_registries.at(index).ref_count;
	if (!ref_count)
		return false;
	++ref_count;
	return true;
}

SLKC_API void Global::unref_ast_node(ast::AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.weak_ref_count) && (!reg.pin_count)) {
		_add_ast_node_to_deferred_deleting_list(reg.in_memory);
		_ast_node_registries.remove(index);
	}
}

SLKC_API void Global::unref_ast_node_weak(ast::AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);
	if ((!--reg.weak_ref_count) && (!reg.ref_count) && (!reg.pin_count)) {
		_add_ast_node_to_deferred_deleting_list(reg.in_memory);
		_ast_node_registries.remove(index);
	}
}

SLKC_API peff::Result<ast::AstNode *, PinFailReason> Global::pin_ast_node(ast::AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);

	++reg.pin_count;

	return +reg.in_memory;
}

SLKC_API void Global::unpin_ast_node(ast::AstNodeIndex index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	auto &reg = _ast_node_registries.at(index);
	if ((!--reg.pin_count) && (!reg.ref_count) && (!reg.weak_ref_count)) {
		_add_ast_node_to_deferred_deleting_list(reg.in_memory);
		_ast_node_registries.remove(index);
	}
}

SLKC_API ast::AstNodeIndex Global::_alloc_ast_node_index() noexcept {
	{
		ast::AstNodeIndex i = _min_free_ast_node_index;
		while (i < std::numeric_limits<ast::AstNodeIndex>::max()) {
			if (!_ast_node_registries.contains(i)) {
				++_min_free_ast_node_index;
				return i;
			}
			if (i < std::numeric_limits<ast::AstNodeIndex>::max() / 2) {
				auto it = _ast_node_registries.find_max_lteq(std::numeric_limits<ast::AstNodeIndex>::max() - i);

				if (it != _ast_node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return ast::INVALID_AST_NODE_INDEX;
}

SLKC_API peff::Option<ast::AstNodeIndex> Global::map_ast_node(ast::AstNode *node, ast::AstNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	if (node_index == ast::INVALID_AST_NODE_INDEX) {
		if ((node_index = _alloc_ast_node_index()) == ast::INVALID_AST_NODE_INDEX)
			return ast::INVALID_AST_NODE_INDEX;
	}

	assert(!this->_ast_node_registries.contains(node_index));

	AstNodeRegistry reg;

	reg.in_memory = node;

	reg.self_index = node_index;

	if (!this->_ast_node_registries.insert(+node_index, std::move(reg)))
		return peff::NULLOPT;

	node->set_node_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_ast_node(ast::AstNodeIndex node_index, ast::AstNode *node) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	assert(this->_ast_node_registries.contains(node_index));

	this->_ast_node_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_ast_node(ast::AstNodeIndex node_index) noexcept {
	std::lock_guard g(this->_ast_node_registries_mutex);

	this->_ast_node_registries.remove(node_index);
}

SLKC_API peff::Result<ast::AstNodeIndex, ast::DuplicationError> Global::duplicate_ast_node(ast::AstNodeIndex node_index) noexcept {
	ast::AstNodeDuplicationContext context(this);

	ast::AstNodePtr<ast::AstNode> node(this, node_index);
	ast::AstNodePin<ast::AstNode> pinned = node.pin();

	ast::AstNodeIndex new_index;
	{
		auto map_result = this->map_ast_node(nullptr);
		if (!map_result.has_value())
			return ast::DuplicationError::OutOfMemory;
		if (map_result.value() == ast::INVALID_AST_NODE_INDEX)
			return ast::DuplicationError::NoSlot;
		new_index = map_result.value();
	}

	auto result = pinned->do_duplicate(context, new_index);

	if (result.is_error())
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

			ast::AstNodePtr<ast::AstNode> dup_node(this, task.src);
			ast::AstNodePin<ast::AstNode> dup_pinned = dup_node.pin();

			auto dup_result = dup_pinned->do_duplicate(context, new_index);

			if (dup_result.is_error())
				return std::move(dup_result).error();

			this->remap_ast_node(task.dest, dup_result.value());

			old_task_list.pop_front();
		}
	}

	unmap_guard.release();

	return new_index;
}

SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::shallow_dump_ast_node(peff::Alloc *allocator, ast::AstNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	ast::AstNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (const auto &i : task_list) {
			ast::AstNodePtr<ast::AstNode> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::deep_dump_ast_node(peff::Alloc *allocator, ast::AstNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	ast::AstNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (const auto &i : task_list) {
			ast::AstNodePtr<ast::AstNode> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, true));
		}
	}

	return root_value.release();
}

SLKC_API bool Global::try_ref_green_node(ast::GreenNodeIndex index) noexcept {
	_clear_zero_ref_green_node_registry_list();
	auto &ref_count = _green_node_registries.at(index).ref_count;
	if (!ref_count)
		return false;
	++ref_count;
	return true;
}

SLKC_API void Global::unref_green_node(ast::GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.weak_ref_count) && (!reg.pin_count)) {
		_add_green_node_to_deferred_deleting_list(reg.in_memory);
		_green_node_registries.remove(index);
	}
}

SLKC_API void Global::unref_green_node_weak(ast::GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);
	if ((!--reg.weak_ref_count) && (!reg.ref_count) && (!reg.pin_count)) {
		_add_green_node_to_deferred_deleting_list(reg.in_memory);
		_green_node_registries.remove(index);
	}
}

SLKC_API peff::Result<ast::GreenNode *, PinFailReason> Global::pin_green_node(ast::GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);

	++reg.pin_count;

	// TODO: Use actual process instead of this.
	return +reg.in_memory;
}

SLKC_API void Global::unpin_green_node(ast::GreenNodeIndex index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	auto &reg = _green_node_registries.at(index);
	if ((!--reg.pin_count) && (!reg.ref_count) && (!reg.weak_ref_count)) {
		_add_green_node_to_deferred_deleting_list(reg.in_memory);
		_green_node_registries.remove(index);
	}
}

SLKC_API ast::GreenNodeIndex Global::_alloc_green_node_index() noexcept {
	{
		ast::GreenNodeIndex i = _min_free_green_node_index;
		while (i < std::numeric_limits<ast::AstNodeIndex>::max()) {
			if (!_green_node_registries.contains(i)) {
				++_min_free_green_node_index;
				return i;
			}
			if (i < std::numeric_limits<ast::AstNodeIndex>::max() / 2) {
				auto it = _green_node_registries.find_max_lteq(std::numeric_limits<ast::AstNodeIndex>::max() - i);

				if (it != _green_node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return ast::INVALID_AST_NODE_INDEX;
}

SLKC_API peff::Option<ast::GreenNodeIndex> Global::map_green_node(ast::GreenNode *node, ast::GreenNodeIndex node_index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	if (node_index == ast::INVALID_GREEN_NODE_INDEX) {
		if ((node_index = _alloc_green_node_index()) == ast::INVALID_GREEN_NODE_INDEX)
			return ast::INVALID_GREEN_NODE_INDEX;
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

SLKC_API void Global::remap_green_node(ast::GreenNodeIndex node_index, ast::GreenNode *node) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	assert(this->_green_node_registries.contains(node_index));

	this->_green_node_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_green_node(ast::GreenNodeIndex node_index) noexcept {
	std::lock_guard g(this->_green_node_registries_mutex);

	this->_green_node_registries.remove(node_index);
}

SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::shallow_dump_green_node(peff::Alloc *allocator, ast::GreenNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	ast::GreenNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			ast::GreenNodePtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_green_node(dump_context, i.dest, pinned_src, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::deep_dump_green_node(peff::Alloc *allocator, ast::GreenNodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	ast::GreenNodeDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			ast::GreenNodePtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_green_node(dump_context, i.dest, pinned_src, true));
		}
	}

	return root_value.release();
}


SLKC_API bool Global::try_ref_type_def(comp::TypeDefIndex index) noexcept {
	_clear_zero_ref_type_def_registry_list();
	auto &ref_count = _type_def_registries.at(index).ref_count;
	if (!ref_count)
		return false;
	++ref_count;
	return true;
}

SLKC_API void Global::unref_type_def(comp::TypeDefIndex index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	auto &reg = _type_def_registries.at(index);
	if ((!--reg.ref_count) && (!reg.weak_ref_count) && (!reg.pin_count)) {
		_add_type_def_to_deferred_deleting_list(reg.in_memory);
		_type_def_registries.remove(index);
	}
}

SLKC_API void Global::unref_type_def_weak(comp::TypeDefIndex index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	auto &reg = _type_def_registries.at(index);
	if ((!--reg.weak_ref_count) && (!reg.ref_count) && (!reg.pin_count)) {
		_add_type_def_to_deferred_deleting_list(reg.in_memory);
		_type_def_registries.remove(index);
	}
}

SLKC_API peff::Result<comp::TypeDef *, PinFailReason> Global::pin_type_def(comp::TypeDefIndex index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	auto &reg = _type_def_registries.at(index);

	++reg.pin_count;

	// TODO: Use actual process instead of this.
	return +reg.in_memory;
}

SLKC_API void Global::unpin_type_def(comp::TypeDefIndex index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	auto &reg = _type_def_registries.at(index);
	if ((!--reg.pin_count) && (!reg.ref_count) && (!reg.weak_ref_count)) {
		_add_type_def_to_deferred_deleting_list(reg.in_memory);
		_type_def_registries.remove(index);
	}
}

SLKC_API comp::TypeDefIndex Global::_alloc_type_def_index() noexcept {
	{
		comp::TypeDefIndex i = _min_free_type_def_index;
		while (i < std::numeric_limits<ast::AstNodeIndex>::max()) {
			if (!_type_def_registries.contains(i)) {
				++_min_free_type_def_index;
				return i;
			}
			if (i < std::numeric_limits<ast::AstNodeIndex>::max() / 2) {
				auto it = _type_def_registries.find_max_lteq(std::numeric_limits<ast::AstNodeIndex>::max() - i);

				if (it != _type_def_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return ast::INVALID_AST_NODE_INDEX;
}

SLKC_API peff::Option<comp::TypeDefIndex> Global::map_type_def(comp::TypeDef *node, comp::TypeDefIndex node_index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	if (node_index == comp::INVALID_TYPE_DEF_INDEX) {
		if ((node_index = _alloc_type_def_index()) == comp::INVALID_TYPE_DEF_INDEX)
			return comp::INVALID_TYPE_DEF_INDEX;
	}

	assert(!this->_type_def_registries.contains(node_index));

	TypeDefRegistry reg;

	reg.in_memory = node;

	reg.self_index = node_index;

	if (!this->_type_def_registries.insert(+node_index, std::move(reg)))
		return peff::NULLOPT;

	node->set_type_def_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_type_def(comp::TypeDefIndex node_index, comp::TypeDef *node) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	assert(this->_type_def_registries.contains(node_index));

	this->_type_def_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_type_def(comp::TypeDefIndex node_index) noexcept {
	std::lock_guard g(this->_type_def_registries_mutex);

	this->_type_def_registries.remove(node_index);
}

/* SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::shallow_dump_type_def(peff::Alloc *allocator, comp::TypeDefIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	TypeDefDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			TypeDefPtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_type_def(dump_context, i.dest, pinned_src, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::deep_dump_type_def(peff::Alloc *allocator, TypeDefIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	TypeDefDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			TypeDefPtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_type_def(dump_context, i.dest, pinned_src, true));
		}
	}

	return root_value.release();
}*/

SLKC_API GlobalSharedString *Global::register_shared_string(std::string_view sv) noexcept {
	std::lock_guard g(_shared_strings_mutex);
	if (auto it = _shared_strings.find_alt(sv); it != _shared_strings.end())
		return &*it;
	char *s = static_cast<char *>(resource_allocator->alloc(sv.size() + 1, alignof(char)));

	if (!s)
		return nullptr;

	peff::ScopeGuard sg([this, s, &sv]() noexcept {
		resource_allocator->release(s, sv.size(), alignof(char));
	});

	memcpy(s, sv.data(), sv.size());
	s[sv.size()] = '\0';

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
	if (_root_module != ast::INVALID_AST_NODE_INDEX)
		return true;

	auto raw_node_ptr = ast::make_ast_node<ast::ModuleNode>(this);
	if (!raw_node_ptr)
		return false;

	_root_module = raw_node_ptr->get_node_index();

	return true;
}
