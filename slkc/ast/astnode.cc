#include "nodedefs/type_base.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API bool ast::is_member_node_type(NodeType node_type) {
	switch (node_type) {
		case NodeType::Class:
		case NodeType::Struct:
		case NodeType::Except:
		case NodeType::Interface:
		case NodeType::Trait:
		case NodeType::ConstEnum:
		case NodeType::ScopedEnum:
		case NodeType::UnionEnum:
		case NodeType::EnumItem:
		case NodeType::UnionEnumItem:
		case NodeType::Attribute:
		case NodeType::Fn:
		case NodeType::FnOverloading:
		case NodeType::Var:
		case NodeType::GenericParam:
		case NodeType::Module:
		case NodeType::Import:
			return true;
		default:
			return false;
	}
}

SLKC_API AstNode::AstNode(NodeType ast_node_type, Global *global)
	: _ast_node_type(ast_node_type),
	  _global(global) {
}

SLKC_API AstNode::AstNode(const AstNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index)
	: _ast_node_type(other._ast_node_type),
	  _global(other._global),
	  _node_index(node_index) {
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API DumpResult AstNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(get_ast_node_type())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("node_type", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API AstNodeDuplicationContext::AstNodeDuplicationContext(Global *global) : global(global), task_list(global->get_allocator()), post_run_hooks(global->get_allocator()) {
}

SLKC_API peff::Result<AstNodeIndex, DuplicationError> AstNodeDuplicationContext::push_task(AstNodeIndex node_index) noexcept {
	if (node_index == INVALID_AST_NODE_INDEX)
		return +INVALID_AST_NODE_INDEX;

	auto result = global->map_ast_node(nullptr);

	if (!result.has_value())
		return DuplicationError::OutOfMemory;

	if (*result == INVALID_AST_NODE_INDEX)
		return DuplicationError::NoSlot;

	task_list.back().dest = *result;

	if (!task_list.push_back({ *result, node_index }))
		return DuplicationError::OutOfMemory;

	return result.move();
}

SLKC_API peff::Result<TypeName, DuplicationError> AstNodeDuplicationContext::push_task(const TypeName &type_name) noexcept {
	auto def = type_name.get_def();
	if (!def)
		return TypeName(type_name);

	auto result_index = this->push_task(def.get_index());

	if (result_index.has_error()) {
		return std::move(result_index).error();
	}

	TypeName tn = type_name;

	tn.set_def(AstNodePtr<TypeNameDefNode>(global, result_index.value()));

	return tn;
}

SLKC_API AstNodeDumpContext::AstNodeDumpContext(
	Global *global,
	peff::Alloc *allocator,
	wandjson::ObjectValue *root_value)
	: global(global),
	  root_value(root_value),
	  task_list(allocator),
	  allocator(allocator),
	  dumped_nodes(allocator) {
}

SLKC_API DumpResult AstNodeDumpContext::push_task(wandjson::ObjectValue *dest, AstNodeIndex src, bool deep) noexcept {
	assert(src != INVALID_AST_NODE_INDEX);

#ifndef _NDEBUG
	assert(!dumped_nodes.contains(src));
	if (!dumped_nodes.insert(+src))
		return DumpResult::OutOfMemory;
#endif

	if (!task_list.push_back({ src, dest, deep }))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}
