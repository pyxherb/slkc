#include "expr.h"

using namespace slkc;
using namespace slkc::ast;

// No need to duplicate, we only do care about declarations in monomorphizations.
SLKC_NULL_AST_DUPLICATE_FN_DEF(ExprNode);

[[nodiscard]] SLKC_API DumpResult ExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(get_expr_kind())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("expr_kind", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ExprNode::ExprNode(ExprKind expr_kind, Global *global)
	: Node(NodeType::Expr, global),
	  _expr_kind(expr_kind) {
}

SLKC_API ExprNode::~ExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ExprNode);

[[nodiscard]] SLKC_API DumpResult UnaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), operand.get_index(), deep_dump));
	if (!target_object->insert("operand", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(unary_op)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("unary_op", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API UnaryExprNode::UnaryExprNode(Global *global)
	: ExprNode(ExprKind::Unary, global) {
}

SLKC_API UnaryExprNode::~UnaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(UnaryExprNode);

[[nodiscard]] SLKC_API DumpResult BinaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), lhs.get_index(), deep_dump));
	if (!target_object->insert("lhs", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), rhs.get_index(), deep_dump));
	if (!target_object->insert("rhs", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(binary_op)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("binary_op", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator_prefix))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator_prefix", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator_infix))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator_infix", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator_suffix))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator_suffix", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API BinaryExprNode::BinaryExprNode(Global *global)
	: ExprNode(ExprKind::Binary, global) {
}

SLKC_API BinaryExprNode::~BinaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BinaryExprNode);

[[nodiscard]] SLKC_API DumpResult TernaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition.get_index(), deep_dump));
	if (!target_object->insert("condition", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), true_branch.get_index(), deep_dump));
	if (!target_object->insert("true_branch", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), false_branch.get_index(), deep_dump));
	if (!target_object->insert("false_branch", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator_question))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator_question", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_operator_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_operator_colon", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API TernaryExprNode::TernaryExprNode(Global *global)
	: ExprNode(ExprKind::Ternary, global) {
}

SLKC_API TernaryExprNode::~TernaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(TernaryExprNode);

[[nodiscard]] SLKC_API DumpResult IdRefExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_id_ref(static_cast<wandjson::ArrayValue *>(v.get()), dump_context, id_ref, deep_dump));
	if (!target_object->insert("id_ref", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API IdRefExprNode::IdRefExprNode(Global *global)
	: ExprNode(ExprKind::IdRef, global),
	  id_ref(global->get_allocator()) {
}

SLKC_API IdRefExprNode::~IdRefExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(IdRefExprNode);

[[nodiscard]] SLKC_API DumpResult HeadedIdRefExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), head_expr.get_index(), deep_dump));
	if (!target_object->insert("head_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_separator))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_separator", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_id_ref(static_cast<wandjson::ArrayValue *>(v.get()), dump_context, id_ref, deep_dump));
	if (!target_object->insert("id_ref", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(Global *global)
	: ExprNode(ExprKind::HeadedIdRef, global),
	  id_ref(global->get_allocator()) {
}

SLKC_API HeadedIdRefExprNode::~HeadedIdRefExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(HeadedIdRefExprNode);

[[nodiscard]] SLKC_API DumpResult I8LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API I8LiteralExprNode::I8LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I8, global) {
}

SLKC_API I8LiteralExprNode::~I8LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I8LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult I16LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API I16LiteralExprNode::I16LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I16, global) {
}

SLKC_API I16LiteralExprNode::~I16LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I16LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult I32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API I32LiteralExprNode::I32LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I32, global) {
}

SLKC_API I32LiteralExprNode::~I32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I32LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult I64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API I64LiteralExprNode::I64LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I64, global) {
}

SLKC_API I64LiteralExprNode::~I64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I64LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult U8LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API U8LiteralExprNode::U8LiteralExprNode(Global *global)
	: ExprNode(ExprKind::U8, global) {
}

SLKC_API U8LiteralExprNode::~U8LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U8LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult U16LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API U16LiteralExprNode::U16LiteralExprNode(Global *global)
	: ExprNode(ExprKind::U16, global) {
}

SLKC_API U16LiteralExprNode::~U16LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U16LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult U32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API U32LiteralExprNode::U32LiteralExprNode(Global *global)
	: ExprNode(ExprKind::U32, global) {
}

SLKC_API U32LiteralExprNode::~U32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U32LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult U64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), std::bit_cast<int64_t>(literal)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API U64LiteralExprNode::U64LiteralExprNode(Global *global)
	: ExprNode(ExprKind::U64, global) {
}

SLKC_API U64LiteralExprNode::~U64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U64LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult F32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_float(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API F32LiteralExprNode::F32LiteralExprNode(Global *global)
	: ExprNode(ExprKind::F32, global) {
}

SLKC_API F32LiteralExprNode::~F32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F32LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult F64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_float(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API F64LiteralExprNode::F64LiteralExprNode(Global *global)
	: ExprNode(ExprKind::F64, global) {
}

SLKC_API F64LiteralExprNode::~F64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F64LiteralExprNode);

[[nodiscard]] SLKC_API DumpResult StringLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(dump_string(dump_context, literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API StringLiteralExprNode::StringLiteralExprNode(Global *global)
	: ExprNode(ExprKind::String, global),
	  literal(global->get_allocator()) {
}

SLKC_API StringLiteralExprNode::~StringLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StringLiteralExprNode);

[[nodiscard]] SLKC_API DumpResult BoolLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("literal", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(Global *global)
	: ExprNode(ExprKind::Bool, global) {
}

SLKC_API BoolLiteralExprNode::~BoolLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BoolLiteralExprNode);

[[nodiscard]] SLKC_API DumpResult NullLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_literal))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_literal", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API NullLiteralExprNode::NullLiteralExprNode(Global *global)
	: ExprNode(ExprKind::Null, global) {
}

SLKC_API NullLiteralExprNode::~NullLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(NullLiteralExprNode);
