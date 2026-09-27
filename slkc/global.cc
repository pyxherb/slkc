#define NOMINMAX
#include "global.h"
#include "ast/rgtree.h"
#include "ast/astnode.h"
#include "comp/type.h"
#include "comp/rg2ast.h"
#include <numeric>

using namespace slkc;

SLKC_API AstNodeRegistry::~AstNodeRegistry() {
}

SLKC_API GreenNodeRegistry::~GreenNodeRegistry() {
}

/* SLKC_API TypeDefRegistry::~TypeDefRegistry() {
}*/

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

SLKC_API void Global::_add_type_def_to_deferred_deleting_list(comp::TypeDef *type_def) noexcept {
	type_def->_next_destructible = _deletable_type_def_registry_list;
	_deletable_type_def_registry_list = type_def;
}

SLKC_API Global::Global(peff::Alloc *allocator) noexcept
	: resource_allocator(allocator),
	  _ast_node_registries(allocator),
	  _green_node_registries(allocator),
	  _registered_type_def_set(allocator),
	  _shared_strings(allocator),
	  _generic_cache_table(allocator) {
}

SLKC_API Global::~Global() noexcept {
	_clear_zero_ref_type_def_registry_list();
	_clear_zero_ref_ast_node_registry_list();
	_clear_zero_ref_green_node_registry_list();
	/* if (_type_def_registries.size())
		std::terminate();*/
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
	while (_deletable_type_def_registry_list) {
		comp::TypeDef *i = _deletable_type_def_registry_list;
		_deletable_type_def_registry_list = nullptr;
		for (comp::TypeDef *next = nullptr; i; i = next) {
			std::lock_guard g(_type_def_registries_mutex);
			next = i->_next_destructible;
			/* if (i->_type_def_index < _min_free_type_def_index)
				_min_free_type_def_index = i->_type_def_index;*/
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

		context.task_list = { get_allocator() };

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

		dump_context.task_list = { get_allocator() };

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

		dump_context.task_list = { get_allocator() };

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

		dump_context.task_list = { get_allocator() };

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

		dump_context.task_list = { get_allocator() };

		for (auto i : task_list) {
			ast::GreenNodePtr node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(dump_green_node(dump_context, i.dest, pinned_src, true));
		}
	}

	return root_value.release();
}

SLKC_API comp::TypeDef *Global::register_type_def(comp::TypeDef *new_type_def) {
	std::lock_guard g(_type_def_registries_mutex);
	if (auto it = _registered_type_def_set.find(new_type_def); it != _registered_type_def_set.end()) {
		(*it)->inc_ref();
		return *it;
	}
	if (!_registered_type_def_set.insert(+new_type_def))
		return nullptr;

	new_type_def->inc_ref();
	return new_type_def;
}

/* SLKC_API bool Global::try_ref_type_def(comp::TypeDefIndex index) noexcept {
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
}*/

/* SLKC_API peff::Result<wandjson::Value *, ast::DumpResult> Global::shallow_dump_type_def(peff::Alloc *allocator, comp::TypeDefIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return ast::DumpResult::OutOfMemory;

	TypeDefDumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_get_allocator() };

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

		dump_context.task_list = { resource_get_allocator() };

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

SLKC_API ast::AstNodePtr<ast::AstNode> Global::lookup_instantiated_generic_ast_node(ast::AstNodeIndex original_node_index, comp::GenericArgListView generic_args) noexcept {
	std::lock_guard g(_generic_cache_mutex);
	if (auto it = _generic_cache_table.find(original_node_index); it != _generic_cache_table.end()) {
		if (auto jt = it.value().find(generic_args); jt != it.value().end()) {
			return ast::AstNodePtr<ast::AstNode>(this, jt.value());
		}
	}
	return {};
}

SLKC_API void Global::remove_instantiated_generic_ast_node(ast::AstNodeIndex original_node_index, comp::GenericArgListView generic_args) noexcept {
	std::lock_guard g(_generic_cache_mutex);
	if (auto it = _generic_cache_table.find(original_node_index); it != _generic_cache_table.end()) {
		if (auto jt = it.value().find(generic_args); jt != it.value().end()) {
			unref_ast_node(jt.value());

			// We unreference the cached nodes manually here, see the notes of the cache types.
			it.value().remove(jt.key());

			if (!it.value().size())
				_generic_cache_table.remove(it.key());
		}
	}
}

SLKC_API void Global::remove_instantiated_generic_ast_node(ast::AstNodeIndex original_node_index) noexcept {
	std::lock_guard g(_generic_cache_mutex);
	if (auto it = _generic_cache_table.find(original_node_index); it != _generic_cache_table.end()) {
		{
			// Also, unreference the cached nodes manually here.
			for (const auto &i : it.value()) {
				unref_ast_node(i.second);
			}
			_generic_cache_table.remove(it.key());
		}
	}
}

struct GenericInstantiationContext;

class GenericInstantiationDispatcher;

struct GenericInstantiationContext {
	peff::RcObjectPtr<peff::Alloc> allocator;
	std::span<ast::AstNodePtr<ast::TypeNameNode>> payload_list;
	peff::HashMap<GlobalSharedStringRef, ast::AstNodePtr<ast::TypeNameNode>> mapped_generic_args;
	ast::AstNodePtr<ast::MemberNode> mapped_node;
	GenericInstantiationDispatcher *dispatcher = nullptr;
	std::atomic_size_t ref_count = 0;

	SLAKE_FORCEINLINE GenericInstantiationContext(
		peff::Alloc *allocator,
		std::span<ast::AstNodePtr<ast::TypeNameNode>> payload_list,
		GenericInstantiationDispatcher *dispatcher)
		: allocator(allocator),
		  payload_list(payload_list),
		  mapped_generic_args(allocator),
		  dispatcher(dispatcher) {
	}

	SLAKE_FORCEINLINE void inc_ref(size_t ignored = 0) {
		++ref_count;
	}

	SLAKE_FORCEINLINE void dec_ref(size_t ignored = 0) {
		if (!--ref_count)
			peff::destroy_and_release<GenericInstantiationContext>(allocator.get(), this, alignof(GenericInstantiationContext));
	}
};

struct MemberGenericInstantiationTask {
	peff::RcObjectPtr<GenericInstantiationContext> context;
	ast::AstNodePtr<ast::MemberNode> member;
};

struct TypeSlotGenericInstantiationTask {
	peff::RcObjectPtr<GenericInstantiationContext> context;
	ast::AstNodePtr<ast::TypeNameNode> &type_name;
};

struct AstNodeGenericInstantiationTask {
	peff::RcObjectPtr<GenericInstantiationContext> context;
	ast::AstNodePtr<ast::TypeNameNode> &node;
};

struct GenericInstantiationDispatcher {
	Global *global;
	peff::List<MemberGenericInstantiationTask> member_tasks;
	peff::List<TypeSlotGenericInstantiationTask> type_tasks;
	peff::List<AstNodeGenericInstantiationTask> ast_node_tasks;
	peff::Set<ast::AstNodePin<ast::FnOverloadingNode>> collected_overloads;
	peff::Set<ast::AstNodePin<ast::FnNode>> collected_fns;

	SLAKE_FORCEINLINE GenericInstantiationDispatcher(Global *global) : global(global), member_tasks(global->get_allocator()), ast_node_tasks(global->get_allocator()), type_tasks(global->get_allocator()), collected_overloads(global->get_allocator()), collected_fns(global->get_allocator()) {}

	[[nodiscard]] SLAKE_FORCEINLINE peff::Option<comp::CompilationError> push_member_task(MemberGenericInstantiationTask &&task) noexcept {
		return member_tasks.push_back(std::move(task)) ? peff::NULLOPT : comp::gen_oom_error_option();
	}

	[[nodiscard]] SLAKE_FORCEINLINE peff::Option<comp::CompilationError> push_type_slot_task(TypeSlotGenericInstantiationTask &&task) noexcept {
		return type_tasks.push_back(std::move(task)) ? peff::NULLOPT : comp::gen_oom_error_option();
	}

	[[nodiscard]] SLAKE_FORCEINLINE peff::Option<comp::CompilationError> push_ast_node_task(AstNodeGenericInstantiationTask &&task) noexcept {
		return ast_node_tasks.push_back(std::move(task)) ? peff::NULLOPT : comp::gen_oom_error_option();
	}
};

static peff::Option<comp::CompilationError> _walk_type_name_for_generic_instantiation(
	ast::AstNodePtr<ast::TypeNameNode> &type_name,
	const GenericInstantiationContext &context);

static peff::Option<comp::CompilationError> _walk_type_name_for_generic_instantiation(
	ast::AstNodePtr<ast::TypeNameNode> &type_name,
	const peff::RcObjectPtr<GenericInstantiationContext> &context) {
	if (!type_name) {
		return peff::NULLOPT;
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->push_type_slot_task(TypeSlotGenericInstantiationTask{ context, type_name }));

	return peff::NULLOPT;
}

static peff::Option<comp::CompilationError> _walk_node_for_generic_instantiation(
	ast::AstNodePtr<ast::MemberNode> ast_node,
	const peff::RcObjectPtr<GenericInstantiationContext> &context) {
	if (!ast_node) {
		return peff::NULLOPT;
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->push_member_task(MemberGenericInstantiationTask{ context, ast_node }));

	return peff::NULLOPT;
}

SLKC_API peff::Option<comp::CompilationError> Global::instantiate_generic_ast_node(const ast::AstNodePin<ast::AstNode> &original_node, comp::GenericArgListView generic_args, ast::AstNodePtr<ast::TypeNameNode> *generic_args_payloads, ast::AstNodePtr<ast::AstNode> &node_out) noexcept {
	if ((node_out = lookup_instantiated_generic_ast_node(original_node.get_index(), generic_args)))
		return peff::NULLOPT;

	std::lock_guard g(_generic_cache_mutex);

	/* {
		bool recursed;
		SLKC_RETURN_IF_COMP_ERROR(is_higher_ranked_cyclic_inherited(shared_from_this(), original_node, recursed));
		if (recursed) {
			ModuleNode *mod = generic_args.back()->token_range.module_node;

			// TODO: Placeholder, use a proper one.
			return CompilationError(TokenRange{ mod, idx_name_token },
				CompilationErrorKind::CyclicInheritedClass);
		}
	}*/

	ast::AstNodePin<ast::AstNode> duplicated_object;

	{
		auto result = duplicate_ast_node(original_node.get_index());
		if (result.is_error()) {
			switch (std::move(result).error()) {
				case ast::DuplicationError::NoSlot:
					return comp::gen_out_of_node_index_error_option();
				case ast::DuplicationError::OutOfMemory:
					return comp::gen_oom_error_option();
				case ast::DuplicationError::PinningFailed:
					return comp::gen_pinning_io_error_option();
			}
			std::terminate();
		}

		ast::AstNodePtr<ast::AstNode> dup = ast::AstNodePtr<ast::AstNode>(this, std::move(result).value());

		if ((duplicated_object = dup.pin()).is_fail())
			return comp::_pin_fail_reason_to_comp_error(duplicated_object.get_fail_reason());
	}

	{
		{
			// Map generic arguments.
			GenericInstantiationDispatcher dispatcher(this);
			peff::RcObjectPtr<GenericInstantiationContext> context;

			if (!(context = peff::alloc_and_construct<GenericInstantiationContext>(get_allocator(), alignof(GenericInstantiationContext), get_allocator(), std::span(generic_args_payloads, generic_args.size()), &dispatcher)))
				return comp::gen_oom_error_option();

			switch (original_node->get_ast_node_type()) {
				case ast::NodeType::Fn: {
					ast::AstNodePin<ast::FnNode> obj = duplicated_object.cast_to<ast::FnNode>();

					peff::DynArray<ast::AstNodePtr<ast::FnOverloadingNode>> overloadings(get_allocator());

					for (auto i : obj->overloadings) {
						context->mapped_node = i.cast_to<ast::MemberNode>();

						auto pinned = i.pin();
						if (pinned.is_fail())
							return comp::_pin_fail_reason_to_comp_error(pinned.get_fail_reason());

						if (generic_args.size() != pinned->get_scope()->generic_params.size())
							continue;

						for (auto [k, v] : pinned->get_scope()->generic_params_index) {
							if (!context->mapped_generic_args.insert(
									GlobalSharedStringRef(k),
									ast::AstNodePtr<ast::TypeNameNode>(generic_args_payloads[v]))) {
								return comp::gen_oom_error_option();
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(i.cast_to<ast::MemberNode>(), context));

						if (!overloadings.push_back(ast::AstNodePtr<ast::FnOverloadingNode>(i))) {
							return comp::gen_oom_error_option();
						}
					fn_overloading_mismatched:;
					}

					if (!overloadings.shrink_to_fit()) {
						return comp::gen_oom_error_option();
					}

					obj->overloadings = std::move(overloadings);

					break;
				}
				case ast::NodeType::Class: {
					ast::AstNodePin<ast::ClassNode> obj = duplicated_object.cast_to<ast::ClassNode>();

					context->mapped_node = obj.cast_to<ast::MemberNode>();

					if (generic_args.size() != obj->get_scope()->generic_params.size()) {
						std::terminate();
						// TODO: return a mismatched generic argument number error.
					}

					for (auto [k, v] : obj->get_scope()->generic_params_index) {
						if (!context->mapped_generic_args.insert(
								GlobalSharedStringRef(k),
								ast::AstNodePtr<ast::TypeNameNode>(generic_args_payloads[v]))) {
							return comp::gen_oom_error_option();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(duplicated_object.cast_to<ast::MemberNode>(), context));
					break;
				}
				case ast::NodeType::Interface: {
					ast::AstNodePin<ast::InterfaceNode> obj = duplicated_object.cast_to<ast::InterfaceNode>();

					context->mapped_node = obj.cast_to<ast::MemberNode>();

					if (generic_args.size() != obj->get_scope()->generic_params.size()) {
						std::terminate();
						// TODO: return a mismatched generic argument number error.
					}

					for (auto [k, v] : obj->get_scope()->generic_params_index) {
						if (!context->mapped_generic_args.insert(
								GlobalSharedStringRef(k),
								ast::AstNodePtr<ast::TypeNameNode>(generic_args_payloads[v]))) {
							return comp::gen_oom_error_option();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(duplicated_object.cast_to<ast::MemberNode>(), context));
					break;
				}
				case ast::NodeType::Struct: {
					ast::AstNodePin<ast::StructNode> obj = duplicated_object.cast_to<ast::StructNode>();

					context->mapped_node = obj.cast_to<ast::MemberNode>();

					if (generic_args.size() != obj->get_scope()->generic_params.size()) {
						std::terminate();
						// TODO: return a mismatched generic argument number error.
					}

					for (auto [k, v] : obj->get_scope()->generic_params_index) {
						if (!context->mapped_generic_args.insert(
								GlobalSharedStringRef(k),
								ast::AstNodePtr<ast::TypeNameNode>(generic_args_payloads[v]))) {
							return comp::gen_oom_error_option();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(duplicated_object.cast_to<ast::MemberNode>(), context));
					break;
				}
				default:
					std::terminate();
					// TODO: return a mismatched generic argument number error.
			}

			while (true) {
				auto type_name_tasks = std::move(dispatcher.type_tasks);
				auto member_tasks = std::move(dispatcher.member_tasks);
				auto ast_node_tasks = std::move(dispatcher.ast_node_tasks);

				dispatcher.type_tasks = { get_allocator() };
				dispatcher.member_tasks = { get_allocator() };
				dispatcher.ast_node_tasks = { get_allocator() };

				if ((!type_name_tasks.size() &&
						(!member_tasks.size())) &&
					(!ast_node_tasks.size()))
					break;

				for (auto &task : type_name_tasks) {
					auto &type_name = task.type_name;

					auto pinned = type_name.pin();
					if (pinned.is_fail())
						return comp::_pin_fail_reason_to_comp_error(pinned.get_fail_reason());

					switch (pinned->get_tn_kind()) {
						case ast::TypeNameKind::Array: {
							ast::AstNodePin<ast::ArrayTypeNameNode> tn = pinned.cast_to<ast::ArrayTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->element_type, task.context));
							break;
						}
						case ast::TypeNameKind::Ref: {
							ast::AstNodePin<ast::RefTypeNameNode> tn = pinned.cast_to<ast::RefTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->element_type, task.context));
							break;
						} /*
						case ast::TypeNameKind::Fn: {
							ast::AstNodePin<ast::FnTypeNameNode> tn = pinned.cast_to<ast::FnTypeNameNode>();

							for (size_t i = 0; i < tn->param_types.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
							}
							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->return_type, task.context));
							break;
						}*/
						case ast::TypeNameKind::Custom: {
							ast::AstNodePin<ast::CustomTypeNameNode> tn = pinned.cast_to<ast::CustomTypeNameNode>();

							if (tn->referred_name.entries.size() == 1) {
								ast::IdRefEntry &entry = tn->referred_name.entries.at(0);

								if (!entry.generic_args.size()) {
									if (auto it = task.context->mapped_generic_args.find(entry.name);
										it != task.context->mapped_generic_args.end()) {
										ast::AstNodePin<ast::TypeNameNode> tn;
										{
											auto result = duplicate_ast_node(it.value().get_index());
											if (result.is_error()) {
												switch (std::move(result).error()) {
													case ast::DuplicationError::NoSlot:
														return comp::gen_out_of_node_index_error_option();
													case ast::DuplicationError::OutOfMemory:
														return comp::gen_oom_error_option();
													case ast::DuplicationError::PinningFailed:
														return comp::gen_pinning_io_error_option();
												}
												std::terminate();
											}

											ast::AstNodePtr<ast::TypeNameNode> dup = ast::AstNodePtr<ast::TypeNameNode>(this, std::move(result).value());

											if ((tn = dup.pin()).is_fail())
												return comp::_pin_fail_reason_to_comp_error(duplicated_object.get_fail_reason());
										}
										tn->set_nullability(pinned->get_nullability());

										type_name = std::move(tn);

										break;
									}
								}
							}

							for (size_t i = 0; i < tn->referred_name.entries.size(); ++i) {
								auto &generic_args = tn->referred_name.entries.at(i).generic_args;
								for (size_t j = 0; j < generic_args.size(); ++j) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(generic_args.at(j), task.context));
								}
							}
							break;
						} /*
						case ast::TypeNameKind::Unpacking: {
							ast::AstNodePin<ast::UnpackingTypeNameNode> tn = pinned.cast_to<ast::UnpackingTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->inner_type_name, task.context));
							break;
						}
						case ast::TypeNameKind::ParamTypeList: {
							ast::AstNodePin<ast::ParamTypeListTypeNameNode> tn = pinned.cast_to<ast::ParamTypeListTypeNameNode>();

							for (size_t i = 0; i < tn->param_types.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
							}
							break;
						}*/
						default:
							break;
					}
				}

				for (auto &task : member_tasks) {
					auto &ast_node = task.member;

					auto pinned = ast_node.pin();
					if (pinned.is_fail())
						return comp::_pin_fail_reason_to_comp_error(pinned.get_fail_reason());

					if (task.context->mapped_node == ast_node) {
						if (!pinned->set_generic_args(generic_args)) {
							return comp::gen_oom_error_option();
						}
					}

					switch (pinned->get_ast_node_type()) {
						case ast::NodeType::FnOverloading: {
							ast::AstNodePin<ast::FnOverloadingNode> fn_slot = pinned.cast_to<ast::FnOverloadingNode>();

							for (auto i : fn_slot->get_scope()->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(i.cast_to<ast::MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (fn_slot->get_scope()->generic_params.size())) {
								peff::RcObjectPtr<GenericInstantiationContext> inner_context;

								if (!(context = peff::alloc_and_construct<GenericInstantiationContext>(get_allocator(), alignof(GenericInstantiationContext), get_allocator(), std::span(generic_args_payloads, generic_args.size()), &dispatcher)))
									return comp::gen_oom_error_option();

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = fn_slot->get_scope()->generic_params_index.find(k);
										it == fn_slot->get_scope()->generic_params_index.end()) {
										if (!inner_context->mapped_generic_args.insert(GlobalSharedStringRef(k), ast::AstNodePtr<ast::TypeNameNode>(v))) {
											return comp::gen_oom_error_option();
										}
									}
								}

								for (auto &i : fn_slot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(i.type, inner_context));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(fn_slot->return_type, inner_context));

								// No need to substitute the function body, we just care about the declaration.
							} else {
								for (auto &i : fn_slot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(i.type, task.context));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(fn_slot->return_type, task.context));
							}

							if (!dispatcher.collected_overloads.contains(fn_slot))
								if (!dispatcher.collected_overloads.insert(std::move(fn_slot)))
									return comp::gen_oom_error_option();
							break;
						}
						case ast::NodeType::Fn: {
							ast::AstNodePin<ast::FnNode> fn_slot = pinned.cast_to<ast::FnNode>();

							for (auto i : fn_slot->overloadings) {
								ast::AstNodePtr<ast::MemberNode> a = i.cast_to<ast::MemberNode>();
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(a, task.context));
							}

							if (!dispatcher.collected_fns.contains(fn_slot))
								if (!dispatcher.collected_fns.insert(std::move(fn_slot)))
									return comp::gen_oom_error_option();
							break;
						}
						case ast::NodeType::Var: {
							ast::AstNodePin<ast::VarNode> var_node = pinned.cast_to<ast::VarNode>();

							// TODO: Complete it after added the 'type' member.
							// SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(var_node->type, task.context));
							break;
						}
						case ast::NodeType::Class: {
							ast::AstNodePin<ast::ClassNode> cls = pinned.cast_to<ast::ClassNode>();

							for (auto j : cls->get_scope()->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<ast::MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->get_scope()->generic_params.size())) {
								peff::RcObjectPtr<GenericInstantiationContext> inner_context;

								if (!(context = peff::alloc_and_construct<GenericInstantiationContext>(get_allocator(), alignof(GenericInstantiationContext), get_allocator(), std::span(generic_args_payloads, generic_args.size()), &dispatcher)))
									return comp::gen_oom_error_option();

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->get_scope()->generic_params_index.find(k);
										it == cls->get_scope()->generic_params_index.end()) {
										if (!inner_context->mapped_generic_args.insert(GlobalSharedStringRef(k), ast::AstNodePtr<ast::TypeNameNode>(v))) {
											return comp::gen_oom_error_option();
										}
									}
								}

								if (auto &t = cls->get_scope()->inherited_type; t)
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(t, inner_context));

								for (auto &k : cls->get_scope()->implemented_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k.type, inner_context));
								}

								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								if (auto &t = cls->get_scope()->inherited_type; t)
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(t, task.context));

								for (auto &k : cls->get_scope()->implemented_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k.type, task.context));
								}

								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case ast::NodeType::Struct: {
							ast::AstNodePin<ast::StructNode> cls = pinned.cast_to<ast::StructNode>();

							for (auto j : cls->get_scope()->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<ast::MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->get_scope()->generic_params.size())) {
								peff::RcObjectPtr<GenericInstantiationContext> inner_context;

								if (!(context = peff::alloc_and_construct<GenericInstantiationContext>(get_allocator(), alignof(GenericInstantiationContext), get_allocator(), std::span(generic_args_payloads, generic_args.size()), &dispatcher)))
									return comp::gen_oom_error_option();

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->get_scope()->generic_params_index.find(k);
										it == cls->get_scope()->generic_params_index.end()) {
										if (!inner_context->mapped_generic_args.insert(GlobalSharedStringRef(k), ast::AstNodePtr<ast::TypeNameNode>(v))) {
											return comp::gen_oom_error_option();
										}
									}
								}
								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case ast::NodeType::Interface: {
							ast::AstNodePin<ast::InterfaceNode> cls = pinned.cast_to<ast::InterfaceNode>();

							for (auto j : cls->get_scope()->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<ast::MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->get_scope()->generic_params.size())) {
								peff::RcObjectPtr<GenericInstantiationContext> inner_context;

								if (!(context = peff::alloc_and_construct<GenericInstantiationContext>(get_allocator(), alignof(GenericInstantiationContext), get_allocator(), std::span(generic_args_payloads, generic_args.size()), &dispatcher)))
									return comp::gen_oom_error_option();

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->get_scope()->generic_params_index.find(k);
										it == cls->get_scope()->generic_params_index.end()) {
										if (!inner_context->mapped_generic_args.insert(GlobalSharedStringRef(k), ast::AstNodePtr<ast::TypeNameNode>(v))) {
											return comp::gen_oom_error_option();
										}
									}
								}

								for (auto &k : cls->get_scope()->implemented_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k.type, inner_context));
								}

								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								for (auto &k : cls->get_scope()->implemented_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k.type, task.context));
								}

								for (auto j : cls->get_scope()->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case ast::NodeType::GenericParam: {
							ast::AstNodePin<ast::GenericParamNode> g = pinned.cast_to<ast::GenericParamNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(g->generic_constraint.inherited_type, task.context));

							for (auto &k : g->generic_constraint.implemented_types) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k.type, task.context));
							}
							break;
						}
						default:;
					}
				}
			}

			/* for (auto fn_slot : dispatcher.collected_overloads) {
			rescan_params:
				for (size_t i = 0; i < fn_slot->params.size(); ++i) {
					auto cur_param = fn_slot->params.at(i);
					ast::AstNodePin<ast::TypeNameNode> cur_param_type = fn_slot->params.at(i)->type;

					if (cur_param_type) {
						if (cur_param_type->get_tn_kind() == ast::TypeNameKind::Unpacking) {
							ast::AstNodePin<ast::UnpackingTypeNameNode> unpacking_type = cur_param_type.cast_to<ast::UnpackingTypeNameNode>();

							if (unpacking_type->inner_type_name->get_tn_kind() == ast::TypeNameKind::ParamTypeList) {
								ast::AstNodePin<ast::ParamTypeListTypeNameNode> inner_type_name = unpacking_type->inner_pinned.cast_to<ast::ParamTypeListTypeNameNode>();

								if (!fn_slot->params.erase_range_and_shrink(i, i + 1))
									return comp::gen_oom_error_option();

								if (!fn_slot->params.insert_range_init(i, inner_type_name->param_types.size())) {
									return comp::gen_oom_error_option();
								}

								for (size_t k = 0; k < inner_type_name->param_types.size(); ++k) {
									ast::AstNodePin<VarNode> p = cur_param->duplicate<VarNode>(get_allocator());

									if (!p) {
										return comp::gen_oom_error_option();
									}

									constexpr static size_t len_name = sizeof("arg_") + (sizeof(size_t) << 1) + 1;
									char name_buf[len_name] = { 0 };

									snprintf(name_buf, len_name - 1, "arg_%.02zx", i + k);

									if (!p->name.build(name_buf)) {
										return comp::gen_oom_error_option();
									}

									p->type = inner_type_name->param_types.at(k);

									fn_slot->params.at(i + k) = p;
								}

								// Note that we use nullptr for we assuming that errors that require a compile task.context will never happen.
								SLKC_RETURN_IF_COMP_ERROR(reindex_fn_params(nullptr, fn_slot));

								if (inner_type_name->has_var_args) {
									if (i + 1 != fn_slot->params.size()) {
										return CompilationError(inner_type_name->token_range, CompilationErrorKind::InvalidVarArgHintDuringInstantiation);
									}

									fn_slot->fn_flags |= FN_VARG;
								}
							}

							goto rescan_params;
						}
					}
				}
			}*/

			// TODO: Do the signature duplication check after we finished all infrastructures.
			/* for (auto fn_slot : dispatcher.collected_fns) {
				for (auto it = fn_slot->overloadings.begin(); it != fn_slot->overloadings.end(); ++it) {
					for (auto jt = it + 1; jt != fn_slot->overloadings.end(); ++jt) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_duplicated(*it, *jt, whether));

						if (whether) {
							ModuleNode *mod = nullptr;
							size_t idx_min_token = SIZE_MAX, idx_max_token = 0;

							for (auto i : *context->generic_args) {
								if (!mod) {
									mod = i->token_range.module_node;
								} else if (i->token_range.module_node != mod)
									std::terminate();
								idx_min_token = (std::min)(i->token_range.begin_index, idx_min_token);
								idx_max_token = (std::max)(i->token_range.end_index, idx_max_token);
							}

							return CompilationError(
								TokenRange(mod, idx_min_token, idx_max_token),
								CompilationErrorKind::FunctionOverloadingDuplicatedDuringInstantiation);
						}
					}
				}
			}*/

			if (!_generic_cache_table.insert(original_node.get_index(), { get_allocator() }))
				return comp::gen_oom_error_option();
			if (!_generic_cache_table.at(original_node.get_index()).insert(std::move(original_node.cast_to<ast::MemberNode>()->get_generic_args()), duplicated_object.get_index())) {
				return comp::gen_oom_error_option();
			}

			ref_ast_node(duplicated_object.get_index());
		}
	}

	node_out = duplicated_object;
	return peff::NULLOPT;
}
