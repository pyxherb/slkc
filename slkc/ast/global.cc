#include "global.h"
#include "nodeutil.h"
#include "nodedefs/type_base.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API DuplicationContext::DuplicationContext(Global *global) : global(global), task_list(global->get_allocator()) {
}

SLKC_API peff::Option<NodeIndex> DuplicationContext::push_task(NodeIndex node_index) noexcept {
	if (!task_list.push_back({ INVALID_NODE_INDEX, node_index }))
		return peff::NULL_OPTION;

	peff::ScopeGuard sg([this]() noexcept {
		task_list.pop_back();
	});

	auto result = global->map_node(nullptr);

	if (!result.has_value())
		return peff::NULL_OPTION;

	if (*result == INVALID_NODE_INDEX)
		return INVALID_NODE_INDEX;

	task_list.back().dest = *result;

	sg.release();

	return result;
}

SLKC_API peff::Option<TypeName> DuplicationContext::push_task(const TypeName &type_name) noexcept {
	auto def = type_name.get_def();
	if (!def)
		return type_name;

	auto result_index = this->push_task(def.get_index());

	if (!result_index.has_value())
		return peff::NULL_OPTION;

	TypeName tn = type_name;

	tn.set_def(NodePtr<TypeNameDefNode>(global, *result_index));

	return tn;
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

SLKC_API void Global::unref_node(NodeIndex index) noexcept {
	auto &reg = _node_registries.at(index);
	if ((!--reg.ref_count) && (!reg.pin_count)) {
		reg.next_zero_ref = _zero_ref_node_registry_list;
		_zero_ref_node_registry_list = &reg;
	}
}

SLKC_API Node *Global::pin_node(NodeIndex index) noexcept {
	auto &reg = _node_registries.at(index);

	++reg.pin_count;

	return reg.in_memory.get();
}

SLKC_API void Global::unpin_node(NodeIndex index) noexcept {
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

SLKC_API peff::Option<NodeIndex> Global::map_node(Node *node) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	NodeIndex node_index;
	if ((node_index = _alloc_node_index()))
		return INVALID_NODE_INDEX;

	if (!map_node(node_index, node))
		return peff::NULL_OPTION;

	return node_index;
}

SLKC_API bool Global::map_node(NodeIndex node_index, Node *node) noexcept {
	std::lock_guard g(this->_node_registries_mutex);

	assert(!this->_node_registries.contains(node_index));

	NodeRegistry reg;

	reg.in_memory = std::unique_ptr<Node, peff::DeallocableDeleter<Node>>(node);

	reg.self_index = node_index;

	if (!this->_node_registries.insert(+node_index, std::move(reg)))
		return false;

	return true;
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

SLKC_API peff::Result<NodeIndex, DuplicationResult> Global::duplicate_node(NodeIndex node_index) noexcept {
	DuplicationContext context(this);

	NodePtr<Node> node(this, node_index);
	NodePin<Node> pinned = node.pin();

	NodeIndex new_index;
	{
		auto map_result = this->map_node(nullptr);
		if (!map_result.has_value())
			return DuplicationResult::OutOfMemory;
		if (map_result.value() == INVALID_NODE_INDEX)
			return DuplicationResult::NoSlot;
		new_index = map_result.value();
	}

	auto result = pinned->do_duplicate(context);

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

			auto dup_result = dup_pinned->do_duplicate(context);

			if (dup_result.has_error())
				return std::move(dup_result).error();

			this->remap_node(task.dest, dup_result.value());

			old_task_list.pop_front();
		}
	}

	unmap_guard.release();

	return new_index;
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::shallow_dump_node(NodeIndex node_index) noexcept {
}

SLKC_API peff::Result<wandjson::Value *, DumpResult> Global::deep_dump_node(NodeIndex node_index) noexcept {
}
