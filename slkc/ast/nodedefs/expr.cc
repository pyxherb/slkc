#include "expr.h"
#include <bit>

using namespace slkc;
using namespace slkc::ast;

SLKC_API DumpResult ExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(get_expr_kind())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("expr_kind", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), is_bad()))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("bad", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ExprNode::ExprNode(ExprKind expr_kind, Global *global)
	: Node(NodeType::Expr, global),
	  _expr_kind(expr_kind),
	  _is_bad(false) {
}

SLKC_API ExprNode::ExprNode(const ExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: Node(other, context, node_index),
	  _expr_kind(other._expr_kind),
	  _is_bad(other._is_bad) {
}

SLKC_API ExprNode::~ExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(UnaryExprNode);

SLKC_API DumpResult UnaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API UnaryExprNode::UnaryExprNode(const UnaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  unary_op(other.unary_op),
	  sti_operator(other.sti_operator) {
	if (error_out.has_value())
		return;

	{
		auto result = context.push_task(other.operand.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		operand = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API UnaryExprNode::~UnaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(UnaryExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(BinaryExprNode)

SLKC_API DumpResult BinaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API BinaryExprNode::BinaryExprNode(const BinaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  binary_op(other.binary_op),
	  sti_operator_prefix(other.sti_operator_prefix),
	  sti_operator_infix(other.sti_operator_infix),
	  sti_operator_suffix(other.sti_operator_suffix) {
	if (error_out.has_value())
		return;

	{
		auto result = context.push_task(other.lhs.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		lhs = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(other.rhs.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		rhs = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API BinaryExprNode::~BinaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BinaryExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(TernaryExprNode);

SLKC_API DumpResult TernaryExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API TernaryExprNode::TernaryExprNode(const TernaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  sti_operator_question(other.sti_operator_question),
	  sti_operator_colon(other.sti_operator_colon) {
	if (error_out.has_value())
		return;
	{
		auto result = context.push_task(other.condition.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		condition = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(other.true_branch.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		true_branch = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(other.false_branch.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		false_branch = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API TernaryExprNode::~TernaryExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(TernaryExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(IdRefExprNode);

SLKC_API DumpResult IdRefExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API IdRefExprNode::IdRefExprNode(const IdRefExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  id_ref(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;
	auto result = id_ref.duplicate(context.get_global()->get_allocator());
	if (!result.has_value()) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	id_ref = std::move(result).value();
}

SLKC_API IdRefExprNode::~IdRefExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(IdRefExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(HeadedIdRefExprNode);

SLKC_API DumpResult HeadedIdRefExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API HeadedIdRefExprNode::HeadedIdRefExprNode(const HeadedIdRefExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  id_ref(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;

	{
		auto result = context.push_task(other.head_expr.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		head_expr = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	auto result = id_ref.duplicate(context.get_global()->get_allocator());
	if (!result.has_value()) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	id_ref = std::move(result).value();
}

SLKC_API HeadedIdRefExprNode::~HeadedIdRefExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(HeadedIdRefExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I8LiteralExprNode);

SLKC_API DumpResult I8LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API I8LiteralExprNode::I8LiteralExprNode(const I8LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API I8LiteralExprNode::~I8LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I8LiteralExprNode);

SLKC_API DumpResult I16LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I16LiteralExprNode);

SLKC_API I16LiteralExprNode::I16LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I16, global) {
}

SLKC_API I16LiteralExprNode::I16LiteralExprNode(const I16LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API I16LiteralExprNode::~I16LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I16LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I32LiteralExprNode);

SLKC_API DumpResult I32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API I32LiteralExprNode::I32LiteralExprNode(const I32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API I32LiteralExprNode::~I32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I32LiteralExprNode);

SLKC_API DumpResult I64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I64LiteralExprNode);

SLKC_API I64LiteralExprNode::I64LiteralExprNode(Global *global)
	: ExprNode(ExprKind::I64, global) {
}

SLKC_API I64LiteralExprNode::I64LiteralExprNode(const I64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API I64LiteralExprNode::~I64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I64LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U8LiteralExprNode);

SLKC_API DumpResult U8LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API U8LiteralExprNode::U8LiteralExprNode(const U8LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API U8LiteralExprNode::~U8LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U8LiteralExprNode);

SLKC_API DumpResult U16LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U16LiteralExprNode);

SLKC_API U16LiteralExprNode::U16LiteralExprNode(Global *global)
	: ExprNode(ExprKind::U16, global) {
}

SLKC_API U16LiteralExprNode::U16LiteralExprNode(const U16LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API U16LiteralExprNode::~U16LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U16LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U32LiteralExprNode);

SLKC_API DumpResult U32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API U32LiteralExprNode::U32LiteralExprNode(const U32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API U32LiteralExprNode::~U32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U32LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U64LiteralExprNode);

SLKC_API DumpResult U64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API U64LiteralExprNode::U64LiteralExprNode(const U64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API U64LiteralExprNode::~U64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U64LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(F32LiteralExprNode);

SLKC_API DumpResult F32LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API F32LiteralExprNode::F32LiteralExprNode(const F32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API F32LiteralExprNode::~F32LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F32LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(F64LiteralExprNode);

SLKC_API DumpResult F64LiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API F64LiteralExprNode::F64LiteralExprNode(const F64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API F64LiteralExprNode::~F64LiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F64LiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(StringLiteralExprNode);

SLKC_API DumpResult StringLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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
	: ExprNode(ExprKind::String, global) {
}

SLKC_API StringLiteralExprNode::StringLiteralExprNode(const StringLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API StringLiteralExprNode::~StringLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StringLiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(BoolLiteralExprNode);

SLKC_API DumpResult BoolLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API BoolLiteralExprNode::BoolLiteralExprNode(const BoolLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  literal(other.literal),
	  sti_literal(other.sti_literal) {
}

SLKC_API BoolLiteralExprNode::~BoolLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BoolLiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(NullLiteralExprNode);

SLKC_API DumpResult NullLiteralExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

SLKC_API NullLiteralExprNode::NullLiteralExprNode(const NullLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index)
	: ExprNode(other, context, node_index),
	  sti_literal(other.sti_literal) {
}

SLKC_API NullLiteralExprNode::~NullLiteralExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(NullLiteralExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(InitializerListExprNode);

SLKC_API DumpResult InitializerListExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("elements", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : elements) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_element_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_element_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API InitializerListExprNode::InitializerListExprNode(Global *global)
	: ExprNode(ExprKind::Null, global),
	  elements(global->get_allocator()),
	  sti_element_separators(global->get_allocator()) {
}

SLKC_API InitializerListExprNode::InitializerListExprNode(const InitializerListExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  elements(context.get_global()->get_allocator()),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace),
	  sti_element_separators(context.get_global()->get_allocator()) {
	if (!elements.resize(other.elements.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (auto i = 0; i < elements.size(); i++) {
		auto result = context.push_task(other.elements[i]);

		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}

		elements[i] = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!sti_element_separators.build(other.sti_element_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API InitializerListExprNode::~InitializerListExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(InitializerListExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(CallExprNode);

SLKC_API DumpResult CallExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), target.get_index(), deep_dump));
	if (!target_object->insert("target", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("args", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : args) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_arg_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_arg_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API CallExprNode::CallExprNode(Global *global)
	: ExprNode(ExprKind::Null, global),
	  args(global->get_allocator()),
	  sti_arg_separators(global->get_allocator()) {
}

SLKC_API CallExprNode::CallExprNode(const CallExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  args(context.get_global()->get_allocator()),
	  sti_arg_separators(context.get_global()->get_allocator()) {
	{
		auto result = context.push_task(other.target.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		target = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!args.resize(other.args.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (auto i = 0; i < args.size(); i++) {
		auto result = context.push_task(other.args[i]);

		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}

		args[i] = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!sti_arg_separators.build(other.sti_arg_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API CallExprNode::~CallExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(CallExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(NewExprNode);

SLKC_API DumpResult NewExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, target_type, deep_dump));
	if (!target_object->insert("target_type", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("args", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : args) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_new_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_new_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_arg_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_arg_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API NewExprNode::NewExprNode(Global *global)
	: ExprNode(ExprKind::Null, global),
	  args(global->get_allocator()),
	  sti_arg_separators(global->get_allocator()) {
}

SLKC_API NewExprNode::NewExprNode(const NewExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  args(context.get_global()->get_allocator()),
	  sti_new_keyword(other.sti_new_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis),
	  sti_arg_separators(context.get_global()->get_allocator()) {
	{
		auto result = context.push_task(other.target_type);
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		target_type = std::move(result).value();
	}

	if (!args.resize(other.args.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (auto i = 0; i < args.size(); i++) {
		auto result = context.push_task(other.args[i]);

		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}

		args[i] = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!sti_arg_separators.build(other.sti_arg_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API NewExprNode::~NewExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(NewExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(AllocaExprNode);

SLKC_API DumpResult AllocaExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, target_type, deep_dump));
	if (!target_object->insert("target_type", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("args", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : args) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_alloca_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_new_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_arg_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_arg_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API AllocaExprNode::AllocaExprNode(Global *global)
	: ExprNode(ExprKind::Null, global),
	  args(global->get_allocator()),
	  sti_arg_separators(global->get_allocator()) {
}

SLKC_API AllocaExprNode::AllocaExprNode(const AllocaExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  args(context.get_global()->get_allocator()),
	  sti_alloca_keyword(other.sti_alloca_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis),
	  sti_arg_separators(context.get_global()->get_allocator()) {
	{
		auto result = context.push_task(other.target_type);
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		target_type = std::move(result).value();
	}

	if (!args.resize(other.args.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (auto i = 0; i < args.size(); i++) {
		auto result = context.push_task(other.args[i]);

		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}

		args[i] = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!sti_arg_separators.build(other.sti_arg_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API AllocaExprNode::~AllocaExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(AllocaExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(CastExprNode);

SLKC_API DumpResult CastExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), operand.get_index(), deep_dump));
	if (!target_object->insert("operand", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, target_type, deep_dump));
	if (!target_object->insert("target_type", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_as_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_new_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_nullable_token))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API CastExprNode::CastExprNode(Global *global)
	: ExprNode(ExprKind::Null, global) {
}

SLKC_API CastExprNode::CastExprNode(const CastExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  is_nullable(other.is_nullable),
	  sti_as_keyword(other.sti_as_keyword),
	  sti_nullable_token(other.sti_nullable_token) {
	{
		auto result = context.push_task(other.target_type);
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		target_type = std::move(result).value();
	}

	{
		auto result = context.push_task(other.operand.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		operand = NodePtr<ExprNode>(context.get_global(), result.value());
	}
}

SLKC_API CastExprNode::~CastExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(CastExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(MatchExprNode);

SLKC_API peff::Result<MatchExprBranch, DuplicationError> MatchExprBranch::do_duplicate(DuplicationContext &context) const noexcept {
	MatchExprBranch branch;

	{
		auto result = context.push_task(pattern.get_index());
		if (result.has_error())
			return std::move(result).error();
		branch.pattern = NodePtr<ExprNode>(context.get_global(), result.value());
	}

	{
		auto result = context.push_task(result_value.get_index());
		if (result.has_error())
			return std::move(result).error();
		branch.result_value = NodePtr<ExprNode>(context.get_global(), result.value());
	}

	branch.sti_case_keyword = sti_case_keyword;
	branch.sti_default_keyword = sti_default_keyword;
	branch.sti_colon = sti_colon;

	return std::move(branch);
}

SLKC_API DumpResult MatchExprBranch::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), pattern.get_index(), deep_dump));
	if (!target_object->insert("pattern", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), result_value.get_index(), deep_dump));
	if (!target_object->insert("result_value", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_case_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_case_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_default_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_default_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_colon", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API DumpResult MatchExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition.get_index(), deep_dump));
	if (!target_object->insert("condition", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, return_type, deep_dump));
	if (!target_object->insert("return_type", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("branches", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : branches) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(i.do_dump(dump_context, static_cast<wandjson::ObjectValue *>(v.get()), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_match_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_match_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_return_type_token))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_return_type_token", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_case_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_case_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API MatchExprNode::MatchExprNode(Global *global)
	: ExprNode(ExprKind::Null, global),
	  branches(global->get_allocator()),
	  sti_case_separators(global->get_allocator()) {
}

SLKC_API MatchExprNode::MatchExprNode(const MatchExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  branches(context.get_global()->get_allocator()),
	  sti_match_keyword(other.sti_match_keyword),
	  sti_return_type_token(other.sti_return_type_token),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace),
	  sti_case_separators(context.get_global()->get_allocator()) {
	{
		auto result = context.push_task(other.condition.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		condition = NodePtr<ExprNode>(context.get_global(), result.value());
	}

	{
		auto result = context.push_task(other.return_type);
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		return_type = std::move(result).value();
	}

	if(!branches.resize(other.branches.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for(size_t i = 0; i < branches.size(); ++i) {
		auto result = other.branches[i].do_duplicate(context);

		if(result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		branches[i] = std::move(result).value();
	}

	if(!sti_case_separators.build(other.sti_case_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API MatchExprNode::~MatchExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(MatchExprNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(GroupExprNode)

SLKC_API DumpResult GroupExprNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(ExprNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), operand.get_index(), deep_dump));
	if (!target_object->insert("operand", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API GroupExprNode::GroupExprNode(Global *global)
	: ExprNode(ExprKind::Group, global) {
}

SLKC_API GroupExprNode::GroupExprNode(const GroupExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: ExprNode(other, context, node_index),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
	{
		auto result = context.push_task(other.operand.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		operand = NodePtr<ExprNode>(context.get_global(), result.value());
	}
}

SLKC_API GroupExprNode::~GroupExprNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(GroupExprNode);
