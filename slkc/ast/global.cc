#define NOMINMAX
#include "global.h"
#include "utils.h"
#include "nodedefs.h"
#include "rgtree.h"
#include <numeric>

using namespace slkc;
using namespace slkc::ast;

SLKC_API RGNodeRegistry::~RGNodeRegistry() {
	if (this->in_memory) {
		auto p = static_cast<RGNode *>(this->in_memory);
		peff::destroy_and_release<RGNode>(p->get_global()->get_allocator(), p, alignof(RGNode));
	}
}

SLKC_API GlobalSharedString::GlobalSharedString() noexcept {
}

SLKC_API GlobalSharedString::~GlobalSharedString() {
	if (_ptr)
		_global->get_allocator()->release(_ptr, _length, alignof(char));
}

SLKC_API void Global::_add_node_to_deferred_deleting_list(NodeRegistry *node_registry) noexcept {
	node_registry->next_zero_ref = _zero_ref_node_registry_list;
	_zero_ref_node_registry_list = node_registry;
}

SLKC_API void Global::_add_rg_node_to_deferred_deleting_list(RGNodeRegistry *rgnode_registry) noexcept {
	rgnode_registry->next_zero_ref = _zero_ref_rg_node_registry_list;
	_zero_ref_rg_node_registry_list = rgnode_registry;
}

SLKC_API Global::Global(peff::Alloc *allocator) noexcept
	: resource_allocator(allocator),
	  _node_registries(allocator),
	  _rg_node_registries(allocator),
	  _shared_strings(allocator) {
}

SLKC_API void Global::_clear_zero_ref_node_registry_list() noexcept {
	while (_zero_ref_node_registry_list) {
		for (NodeRegistry *i = _zero_ref_node_registry_list; i; i = i->next_zero_ref) {
			std::lock_guard g(_node_registries_mutex);
			peff::destroy_and_release<NodeRegistry>(this->resource_allocator.get(), i, alignof(NodeRegistry));
			if (i->self_index < _min_free_node_index)
				_min_free_node_index = i->self_index;
		}
	}
}

SLKC_API void Global::_clear_zero_ref_rg_node_registry_list() noexcept {
	while (_zero_ref_rg_node_registry_list) {
		for (RGNodeRegistry *i = _zero_ref_rg_node_registry_list; i; i = i->next_zero_ref) {
			std::lock_guard g(_rg_node_registries_mutex);
			peff::destroy_and_release<RGNodeRegistry>(this->resource_allocator.get(), i, alignof(RGNodeRegistry));
			if (i->self_index < _min_free_rg_node_index)
				_min_free_rg_node_index = i->self_index;
		}
	}
}

SLKC_API void Global::unref_node(NodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.pin_count)) {
		reg.next_zero_ref = _zero_ref_node_registry_list;
		_zero_ref_node_registry_list = &reg;
	}
}

SLKC_API peff::Result<Node *, PinFailReason> Global::pin_node(NodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _node_registries.at(index);

	++reg.pin_count;

	return reg.in_memory.get();
}

SLKC_API void Global::unpin_node(NodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _node_registries.at(index);
	if ((!reg.ref_count) && (!--reg.pin_count)) {
		reg.next_zero_ref = _zero_ref_node_registry_list;
		_zero_ref_node_registry_list = &reg;
	}
}

SLKC_API NodeIndex Global::_alloc_node_index() noexcept {
	NodeIndex new_id = std::numeric_limits<NodeIndex>::max();

	{
		NodeIndex i = _min_free_node_index;
		while (i < std::numeric_limits<NodeIndex>::max()) {
			if (!_node_registries.contains(i)) {
				new_id = i;
				++_min_free_node_index;
				return INVALID_NODE_INDEX;
			}
			if (i < std::numeric_limits<NodeIndex>::max() / 2) {
				auto it = _node_registries.find_max_lteq(std::numeric_limits<NodeIndex>::max() - i);

				if (it != _node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return INVALID_NODE_INDEX;
}

SLKC_API peff::Option<NodeIndex> Global::map_node(Node *node, NodeIndex node_index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	if (node_index == INVALID_NODE_INDEX) {
		if ((node_index = _alloc_rg_node_index()))
			return INVALID_NODE_INDEX;
	}

	assert(!this->_node_registries.contains(node_index));

	NodeRegistry reg;

	reg.in_memory = std::unique_ptr<Node, peff::DeallocableDeleter<Node>>(node);

	reg.self_index = node_index;

	if (!this->_node_registries.insert(+node_index, std::move(reg)))
		return false;

	node->set_node_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_node(NodeIndex node_index, Node *node) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	assert(this->_node_registries.contains(node_index));

	this->_node_registries.at(node_index).in_memory = std::unique_ptr<Node, peff::DeallocableDeleter<Node>>(node);
}

SLKC_API void Global::unmap_node(NodeIndex node_index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	this->_node_registries.remove(node_index);
}

SLKC_API void Global::unref_rg_node(NodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _rg_node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.pin_count)) {
		reg.next_zero_ref = _zero_ref_rg_node_registry_list;
		_zero_ref_rg_node_registry_list = &reg;
	}
}

SLKC_API peff::Result<RGNode *, PinFailReason> Global::pin_rg_node(RGNodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _rg_node_registries.at(index);

	++reg.pin_count;

	// TODO: Use actual process instead of this.
	return static_cast<RGNode *>(reg.in_memory);
}

SLKC_API void Global::unpin_rg_node(RGNodeIndex index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	auto &reg = _rg_node_registries.at(index);
	if ((!reg.ref_count) && (!--reg.pin_count)) {
		reg.next_zero_ref = _zero_ref_rg_node_registry_list;
		_zero_ref_rg_node_registry_list = &reg;
	}
}

SLKC_API RGNodeIndex Global::_alloc_rg_node_index() noexcept {
	RGNodeIndex new_id = std::numeric_limits<NodeIndex>::max();

	{
		NodeIndex i = _min_free_node_index;
		while (i < std::numeric_limits<NodeIndex>::max()) {
			if (!_node_registries.contains(i)) {
				new_id = i;
				++_min_free_node_index;
				return INVALID_NODE_INDEX;
			}
			if (i < std::numeric_limits<NodeIndex>::max() / 2) {
				auto it = _node_registries.find_max_lteq(std::numeric_limits<NodeIndex>::max() - i);

				if (it != _node_registries.end()) {
					i = it.value().self_index + 1;
				} else {
					// This is impossible.
					std::terminate();
				}
			}
		}
	}

	return INVALID_NODE_INDEX;
}

SLKC_API peff::Option<RGNodeIndex> Global::map_rg_node(RGNode *node, RGNodeIndex node_index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	if (node_index == INVALID_RGNODE_INDEX) {
		if ((node_index = _alloc_rg_node_index()))
			return INVALID_RGNODE_INDEX;
	}

	assert(!this->_rg_node_registries.contains(node_index));

	RGNodeRegistry reg;

	reg.in_memory = node;

	reg.self_index = node_index;

	if (!this->_rg_node_registries.insert(+node_index, std::move(reg)))
		return INVALID_RGNODE_INDEX;

	node->set_node_index(node_index);

	return node_index;
}

SLKC_API void Global::remap_rg_node(RGNodeIndex node_index, RGNode *node) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	assert(this->_rg_node_registries.contains(node_index));

	this->_rg_node_registries.at(node_index).in_memory = node;
}

SLKC_API void Global::unmap_rg_node(RGNodeIndex node_index) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	this->_rg_node_registries.remove(node_index);
}

SLKC_API peff::Result<NodeIndex, DuplicationError> Global::duplicate_node(NodeIndex node_index) noexcept {
	DuplicationContext context(this);

	NodePtr<Node> node(this, node_index);
	NodePin<Node> pinned = node.pin();

	NodeIndex new_index;
	{
		auto map_result = this->map_node(nullptr);
		if (!map_result.has_value())
			return DuplicationError::OutOfMemory;
		if (map_result.value() == INVALID_NODE_INDEX)
			return DuplicationError::NoSlot;
		new_index = map_result.value();
	}

	auto result = pinned->do_duplicate(context, new_index);

	if (result.has_error())
		return std::move(result).error();

	this->remap_node(new_index, result.value());

	peff::ScopeGuard unmap_guard([this, new_index]() noexcept {
		this->unmap_node(new_index);
	});

	while (!context.task_list.size()) {
		auto old_task_list = std::move(context.task_list);

		context.task_list = { resource_allocator.get() };

		while (old_task_list.size()) {
			auto task = old_task_list.front();

			NodePtr<Node> dup_node(this, task.src);
			NodePin<Node> dup_pinned = dup_node.pin();

			auto dup_result = dup_pinned->do_duplicate(context, new_index);

			if (dup_result.has_error())
				return std::move(dup_result).error();

			this->remap_node(task.dest, dup_result.value());

			old_task_list.pop_front();
		}
	}

	unmap_guard.release();

	return new_index;
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::shallow_dump_node(peff::Alloc *allocator, NodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	DumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, false));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			NodePtr<Node> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, false));
		}
	}

	return root_value.release();
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::deep_dump_node(peff::Alloc *allocator, NodeIndex node_index) noexcept {
	std::unique_ptr<wandjson::ObjectValue, wandjson::ValueDeleter> root_value(wandjson::ObjectValue::alloc(allocator));

	if (!root_value)
		return DumpResult::OutOfMemory;

	DumpContext dump_context(this, allocator, root_value.get());
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(root_value.get(), node_index, true));

	while (dump_context.task_list.size()) {
		auto task_list = std::move(dump_context.task_list);

		dump_context.task_list = { resource_allocator.get() };

		for (auto i : task_list) {
			NodePtr<Node> node_ptr(this, i.src);
			auto pinned_src = node_ptr.pin();
			SLKC_RETURN_IF_DUMP_FAILED(pinned_src->do_dump(dump_context, i.dest, true));
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

	GlobalSharedString ss;

	ss._global = this;
	ss._length = sv.size();
	ss._ptr = s;
	ss._ref_count = 0;

	if (!_shared_strings.insert(std::move(ss)))
		return nullptr;

	return &_shared_strings.at_alt(sv);
}

SLKC_API void Global::unregister_shared_string(std::string_view s) noexcept {
	std::lock_guard g(_shared_strings_mutex);
	_shared_strings.remove_alt(s);
}

SLKC_API bool Global::init_root_module() noexcept {
	if (_root_module != INVALID_NODE_INDEX)
		return true;

	auto raw_node_ptr = make_node<ModuleNode>(this);
	if (!raw_node_ptr)
		return false;

	_root_module = raw_node_ptr->get_node_index();

	return true;
}
