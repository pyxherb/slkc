#include "nodedefs/type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API Node::Node(NodeType ast_node_type, Global *global)
	: _ast_node_type(ast_node_type),
	  _global(global) {
}

SLKC_API Node::Node(const Node &other, DuplicationContext &context)
	: _ast_node_type(other._ast_node_type),
	  _global(other._global),
	  _token_range(other._token_range) {
}

SLKC_API Node::~Node() {
}

SLKC_API DumpResult Node::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(get_ast_node_type())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("node_type", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API DuplicationContext::DuplicationContext(Global *global) : global(global), task_list(global->get_allocator()), post_run_hooks(global->get_allocator()) {
}

SLKC_API peff::Result<NodeIndex, DuplicationResult> DuplicationContext::push_task(NodeIndex node_index) noexcept {
	if (!task_list.push_back({ INVALID_NODE_INDEX, node_index }))
		return DuplicationResult::OutOfMemory;

	peff::ScopeGuard sg([this]() noexcept {
		task_list.pop_back();
	});

	auto result = global->map_node(nullptr);

	if (!result.has_value())
		return DuplicationResult::OutOfMemory;

	if (*result == INVALID_NODE_INDEX)
		return DuplicationResult::NoSlot;

	task_list.back().dest = *result;

	sg.release();

	return result.move();
}

SLKC_API peff::Result<TypeName, DuplicationResult> DuplicationContext::push_task(const TypeName &type_name) noexcept {
	auto def = type_name.get_def();
	if (!def)
		return TypeName(type_name);

	auto result_index = this->push_task(def.get_index());

	if(result_index.has_error()) {
		return std::move(result_index).error();
	}

	TypeName tn = type_name;

	tn.set_def(NodePtr<TypeNameDefNode>(global, result_index.value()));

	return tn;
}

SLKC_API DumpContext::DumpContext(
	Global *global,
	peff::Alloc *allocator,
	wandjson::ObjectValue *root_value)
	: global(global),
	  root_value(root_value),
	  task_list(allocator),
	  allocator(allocator) {
}

SLKC_API bool DumpContext::push_task(wandjson::ObjectValue *dest, NodeIndex src) noexcept {
	if (!task_list.push_back({ src, dest }))
		return false;

	return true;
}
