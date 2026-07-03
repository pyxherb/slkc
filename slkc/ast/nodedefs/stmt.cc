#include "stmt.h"

using namespace slkc;
using namespace slkc::ast;

// No need to duplicate, we only do care about declarations in monomorphizations.
SLKC_NULL_AST_DUPLICATE_FN_DEF(StmtNode);

SLKC_API DumpResult StmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

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

SLKC_API StmtNode::StmtNode(StmtKind stmt_kind, Global *global, TokenIndex token_index)
	: Node(NodeType::Expr, global, token_index),
	  _stmt_kind(stmt_kind),
	  _is_bad(false) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StmtNode);

SLKC_API DumpResult ExprStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), inner_expr.get_index(), deep_dump));
	if (!target_object->insert("inner_expr", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ExprStmtNode::ExprStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Expr, global, token_index) {
}

SLKC_API ExprStmtNode::~ExprStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ExprStmtNode);

SLKC_API DumpResult BindingEntry::dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::StringValue::alloc(dump_context.get_allocator(), name.get()))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("name", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, type, deep_dump));
	if (!target_object->insert("type", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), initial_value.get_index(), deep_dump));
	if (!target_object->insert("value", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name_token))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name_token_index", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_colon_token_index", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_assignment))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_assign_token_index", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API peff::Result<BindingEntry, DuplicationError> BindingEntry::duplicate(DuplicationContext &context) const noexcept {
	BindingEntry entry;

	entry.name = name;
	{
		auto result = context.push_task(type);
		if (!result.has_error()) {
			entry.type = std::move(result).value();
		} else {
			return std::move(result).error();
		}
	}
	{
		auto result = context.push_task(initial_value);
		if (!result.has_error()) {
			entry.initial_value = NodePtr<ExprNode>(context.get_global(), std::move(result).value());
		} else {
			return std::move(result).error();
		}
	}
	entry.sti_name_token = sti_name_token;
	entry.sti_colon = sti_colon;
	entry.sti_assignment = sti_assignment;

	return std::move(entry);
}

SLKC_API DumpResult LetStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("elements", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : bindings) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(i.dump(dump_context, static_cast<wandjson::ObjectValue *>(v.get()), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_let_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_let_keyword", v.release()))
		return DumpResult::OutOfMemory;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_element_separators", v.release()))
			return DumpResult::OutOfMemory;

		for (auto i : sti_binding_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API LetStmtNode::LetStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Let, global, token_index),
	  bindings(global->get_allocator()),
	  sti_binding_separators(global->get_allocator()) {
}

SLKC_API LetStmtNode::~LetStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(LetStmtNode);

SLKC_API DumpResult BreakStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_break_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_break_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API BreakStmtNode::BreakStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Break, global, token_index) {
}

SLKC_API BreakStmtNode::~BreakStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BreakStmtNode);

SLKC_API DumpResult ContinueStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("args", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : continue_values) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_continue_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_continue_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
	for (auto i : sti_continue_values_separators) {
		if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
			return DumpResult::OutOfMemory;
		if (!av->push_back(v.release()))
			return DumpResult::OutOfMemory;
	}
	if (!target_object->insert("sti_continue_values_separators", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ContinueStmtNode::ContinueStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Continue, global, token_index),
	  continue_values(global->get_allocator()),
	  sti_continue_values_separators(global->get_allocator()) {
}

SLKC_API ContinueStmtNode::~ContinueStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ContinueStmtNode);

SLKC_API DumpResult ForStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("elements", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : loop_vars) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(i.dump(dump_context, static_cast<wandjson::ObjectValue *>(v.get()), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition_expr.get_index(), deep_dump));
	if (!target_object->insert("condition_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), step_expr.get_index(), deep_dump));
	if (!target_object->insert("step_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
	if (!target_object->insert("body", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_for_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_for_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_first_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_first_semicolon_index", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_second_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_second_semicolon_index", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ForStmtNode::ForStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::For, global, token_index),
	  loop_vars(global->get_allocator()) {
}

SLKC_API ForStmtNode::~ForStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ForStmtNode);

SLKC_API DumpResult ForEachStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::StringValue::alloc(dump_context.get_allocator(), loop_var_name.get()))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("loop_var_name", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), collection_expr.get_index(), deep_dump));
	if (!target_object->insert("collection_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
	if (!target_object->insert("body", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_foreach_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_foreach_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_colon_index))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_colon_index", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_parenthesis", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ForEachStmtNode::ForEachStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::ForEach, global, token_index) {
}

SLKC_API ForEachStmtNode::~ForEachStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ForEachStmtNode);

SLKC_API DumpResult WhileStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition_expr.get_index(), deep_dump));
	if (!target_object->insert("condition_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
	if (!target_object->insert("body", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_while_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_while_keyword", v.release()))
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

SLKC_API WhileStmtNode::WhileStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::While, global, token_index) {
}

SLKC_API WhileStmtNode::~WhileStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(WhileStmtNode);

SLKC_API DumpResult DoWhileStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition_expr.get_index(), deep_dump));
	if (!target_object->insert("condition_expr", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
	if (!target_object->insert("body", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_do_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_do_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_while_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_while_keyword", v.release()))
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

SLKC_API DoWhileStmtNode::DoWhileStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::DoWhile, global, token_index) {
}

SLKC_API DoWhileStmtNode::~DoWhileStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(DoWhileStmtNode);

SLKC_API DumpResult ReturnStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), return_value.get_index(), deep_dump));
	if (!target_object->insert("return_value", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_return_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_return_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_semicolon", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ReturnStmtNode::ReturnStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Return, global, token_index) {
}

SLKC_API ReturnStmtNode::~ReturnStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ReturnStmtNode);

SLKC_API DumpResult YieldStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), return_value.get_index(), deep_dump));
	if (!target_object->insert("return_value", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_yield_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_yield_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_semicolon", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API YieldStmtNode::YieldStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Yield, global, token_index) {
}

SLKC_API YieldStmtNode::~YieldStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(YieldStmtNode);

SLKC_API DumpResult IfStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

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

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_if_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_if_keyword", v.release()))
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

SLKC_API IfStmtNode::IfStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::If, global, token_index) {
}

SLKC_API IfStmtNode::~IfStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(IfStmtNode);

SLKC_API DumpResult SwitchStmtBranch::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), pattern.get_index(), deep_dump));
	if (!target_object->insert("pattern", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
	if (!target_object->insert("body", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_case_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_case_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_default_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_default_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API DumpResult SwitchStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), condition.get_index(), deep_dump));
	if (!target_object->insert("condition", v.release()))
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

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_switch_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_switch_keyword", v.release()))
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

SLKC_API SwitchStmtNode::SwitchStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Switch, global, token_index),
	  branches(global->get_allocator()) {
}

SLKC_API SwitchStmtNode::~SwitchStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(SwitchStmtNode);

SLKC_API DumpResult BlockStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("inner_stmts", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : inner_stmts) {
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

	return DumpResult::Ok;
}

SLKC_API BlockStmtNode::BlockStmtNode(Global *global, TokenIndex token_index)
	: StmtNode(StmtKind::Block, global, token_index),
	  inner_stmts(global->get_allocator()) {
}

SLKC_API BlockStmtNode::~BlockStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BlockStmtNode);
