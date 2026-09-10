#include "member.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API DumpResult MemberNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (_parent_node_index != INVALID_AST_NODE_INDEX) {
		if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), _parent_node_index))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("outer_node_index", v.release()))
			return DumpResult::OutOfMemory;
	}

	if (self_name) {
		if (!(v = decltype(v)(dump_string(dump_context, self_name))))
			return DumpResult::OutOfMemory;
	} else {
		if (!(v = decltype(v)(dump_string(dump_context, ""))))
			return DumpResult::OutOfMemory;
	}
	if (!target_object->insert("self_name", v.release()))
		return DumpResult::OutOfMemory;

	if (_self_scope) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_scope(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, _self_scope.get(), deep_dump));
		if (!target_object->insert("self_scope", v.release()))
			return DumpResult::OutOfMemory;
	}

	return DumpResult::Ok;
}

SLKC_API MemberNode::MemberNode(NodeType ast_node_type, Global *global)
	: AstNode(ast_node_type, global) {
}

SLKC_API MemberNode::MemberNode(
	const MemberNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: AstNode(other, context, node_index),
	  self_name(other.self_name),
	  access_modifier(other.access_modifier) {
	if (other.member_relative_location.has_value())
		member_relative_location = other.member_relative_location.value();
}

SLKC_API MemberNode::~MemberNode() {
}

SLKC_API bool MemberNode::alloc_scope() noexcept {
	if (!(_self_scope = Scope::alloc(this->get_node_index(), this->get_global())))
		return false;
	return true;
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(MemberNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ModuleNode);

SLKC_API DumpResult ModuleNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_module_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_module_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_module_decl_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_module_decl_semicolon", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ModuleNode::ModuleNode(Global *global)
	: MemberNode(NodeType::Class, global) {
}

SLKC_API ModuleNode::ModuleNode(
	const ModuleNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_module_keyword(other.sti_module_keyword),
	  sti_module_decl_semicolon(other.sti_module_decl_semicolon) {
	if (error_out.has_value())
		return;
}

SLKC_API ModuleNode::~ModuleNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ModuleNode);
