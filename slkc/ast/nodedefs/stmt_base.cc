#include "stmt.h"

using namespace slkc;
using namespace slkc::ast;

// No need to duplicate, we only do care about declarations in monomorphizations.
SLKC_API DumpResult StmtNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(get_stmt_kind())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("stmt_kind", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), is_bad()))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("bad", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API StmtNode::StmtNode(StmtKind stmt_kind, Global *global)
	: AstNode(NodeType::Expr, global),
	  _stmt_kind(stmt_kind),
	  _is_bad(false) {
}

SLKC_API StmtNode::StmtNode(const StmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index)
	: AstNode(other, context, node_index),
	  _stmt_kind(other._stmt_kind),
	  _is_bad(other._is_bad) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StmtNode);
