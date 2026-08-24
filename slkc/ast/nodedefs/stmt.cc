#include "stmt.h"

using namespace slkc;
using namespace slkc::ast;

// No need to duplicate, we only do care about declarations in monomorphizations.
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

SLKC_API StmtNode::StmtNode(StmtKind stmt_kind, Global *global)
	: Node(NodeType::Expr, global),
	  _stmt_kind(stmt_kind),
	  _is_bad(false) {
}

SLKC_API StmtNode::StmtNode(const StmtNode &other, DuplicationContext &context, AstNodeIndex node_index)
	: Node(other, context, node_index),
	  _stmt_kind(other._stmt_kind),
	  _is_bad(other._is_bad) {
}

SLKC_API StmtNode::~StmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ExprStmtNode);

SLKC_API DumpResult ExprStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("inner_exprs", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : inner_exprs) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}

SLKC_API ExprStmtNode::ExprStmtNode(Global *global)
	: StmtNode(StmtKind::Expr, global),
	  inner_exprs(global->get_allocator()) {
}

SLKC_API ExprStmtNode::ExprStmtNode(const ExprStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  inner_exprs(context.get_global()->get_allocator()) {
	if (!inner_exprs.resize(other.inner_exprs.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < inner_exprs.size(); i++) {
		auto result = context.push_task(other.inner_exprs[i].get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		inner_exprs[i] = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
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
			entry.initial_value = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
		} else {
			return std::move(result).error();
		}
	}
	entry.sti_name_token = sti_name_token;
	entry.sti_colon = sti_colon;
	entry.sti_assignment = sti_assignment;

	return std::move(entry);
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(VarDefStmtNode);

SLKC_API DumpResult VarDefStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
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

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<int>(binding_type)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("binding_type", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API VarDefStmtNode::VarDefStmtNode(Global *global)
	: StmtNode(StmtKind::Let, global),
	  bindings(global->get_allocator()),
	  sti_binding_separators(global->get_allocator()) {
}

SLKC_API VarDefStmtNode::VarDefStmtNode(const VarDefStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  bindings(context.get_global()->get_allocator()),
	  sti_binding_separators(context.get_global()->get_allocator()) {
	if (!bindings.resize(other.bindings.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < bindings.size(); ++i) {
		auto result = other.bindings[i].duplicate(context);
		if (result.has_error()) {
			error_out = result.error();
			return;
		}
		bindings[i] = std::move(result).value();
	}

	binding_type = other.binding_type;
}

SLKC_API VarDefStmtNode::~VarDefStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(VarDefStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(BreakStmtNode);

SLKC_API DumpResult BreakStmtNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(StmtNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_break_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_break_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API BreakStmtNode::BreakStmtNode(Global *global)
	: StmtNode(StmtKind::Break, global) {
}

SLKC_API BreakStmtNode::BreakStmtNode(const BreakStmtNode &other, DuplicationContext &context, AstNodeIndex node_index)
	: StmtNode(other, context, node_index),
	  sti_break_keyword(other.sti_break_keyword),
	  sti_semicolon(other.sti_semicolon) {
}

SLKC_API BreakStmtNode::~BreakStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BreakStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ContinueStmtNode);

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

SLKC_API ContinueStmtNode::ContinueStmtNode(Global *global)
	: StmtNode(StmtKind::Continue, global),
	  continue_values(global->get_allocator()),
	  sti_continue_values_separators(global->get_allocator()) {
}

SLKC_API ContinueStmtNode::ContinueStmtNode(const ContinueStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  continue_values(context.get_global()->get_allocator()),
	  sti_continue_values_separators(context.get_global()->get_allocator()),
	  sti_continue_keyword(other.sti_continue_keyword),
	  sti_semicolon(other.sti_semicolon) {
	if (!continue_values.resize(other.continue_values.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	for (size_t i = 0; i < continue_values.size(); ++i) {
		auto result = context.push_task(other.continue_values[i].get_index());
		if (result.has_error()) {
			error_out = result.error();
			return;
		}
		continue_values[i] = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	if (!sti_continue_values_separators.build(other.sti_continue_values_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API ContinueStmtNode::~ContinueStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ContinueStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ForStmtNode);

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

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("step_exprs", v.release()))
			return DumpResult::OutOfMemory;

		for (const auto &i : step_exprs) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

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

SLKC_API ForStmtNode::ForStmtNode(Global *global)
	: StmtNode(StmtKind::For, global),
	  loop_vars(global->get_allocator()),
	  step_exprs(global->get_allocator()) {
}

SLKC_API ForStmtNode::ForStmtNode(const ForStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  loop_vars(context.get_global()->get_allocator()),
	  step_exprs(context.get_global()->get_allocator()),
	  sti_for_keyword(other.sti_for_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_first_semicolon(other.sti_first_semicolon),
	  sti_second_semicolon(other.sti_second_semicolon),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
	if (!loop_vars.resize(other.loop_vars.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	for (size_t i = 0; i < loop_vars.size(); ++i) {
		auto result = loop_vars[i].duplicate(context);
		if (result.has_error()) {
			error_out = result.error();
			return;
		}
		loop_vars[i] = std::move(result).value();
	}
	{
		auto result = context.push_task(other.condition_expr.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		condition_expr = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!step_exprs.resize(other.step_exprs.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < step_exprs.size(); i++) {
		auto result = context.push_task(other.step_exprs[i].get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		step_exprs[i] = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	{
		auto result = context.push_task(other.body.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		body = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API ForStmtNode::~ForStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ForStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ForEachStmtNode);

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

SLKC_API ForEachStmtNode::ForEachStmtNode(Global *global)
	: StmtNode(StmtKind::ForEach, global) {
}

SLKC_API ForEachStmtNode::ForEachStmtNode(const ForEachStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  loop_var_name(other.loop_var_name),
	  collection_expr(other.collection_expr),
	  body(other.body),
	  sti_foreach_keyword(other.sti_foreach_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_colon_index(other.sti_colon_index),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
}

SLKC_API ForEachStmtNode::~ForEachStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ForEachStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(WhileStmtNode);

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

SLKC_API WhileStmtNode::WhileStmtNode(Global *global)
	: StmtNode(StmtKind::While, global) {
}

SLKC_API WhileStmtNode::WhileStmtNode(const WhileStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  sti_while_keyword(other.sti_while_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
	{
		auto result = context.push_task(condition_expr.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		condition_expr = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(body.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		body = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API WhileStmtNode::~WhileStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(WhileStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(DoWhileStmtNode);

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

SLKC_API DoWhileStmtNode::DoWhileStmtNode(Global *global)
	: StmtNode(StmtKind::DoWhile, global) {
}

SLKC_API DoWhileStmtNode::DoWhileStmtNode(const DoWhileStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  sti_do_keyword(other.sti_do_keyword),
	  sti_while_keyword(other.sti_while_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
	{
		auto result = context.push_task(condition_expr.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		condition_expr = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(body.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		body = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API DoWhileStmtNode::~DoWhileStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(DoWhileStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ReturnStmtNode);

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

SLKC_API ReturnStmtNode::ReturnStmtNode(Global *global)
	: StmtNode(StmtKind::Return, global) {
}

SLKC_API ReturnStmtNode::ReturnStmtNode(const ReturnStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  sti_return_keyword(other.sti_return_keyword),
	  sti_semicolon(other.sti_semicolon) {
	{
		auto result = context.push_task(return_value.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		return_value = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API ReturnStmtNode::~ReturnStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ReturnStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(YieldStmtNode);

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

SLKC_API YieldStmtNode::YieldStmtNode(Global *global)
	: StmtNode(StmtKind::Yield, global) {
}

SLKC_API YieldStmtNode::YieldStmtNode(const YieldStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  sti_yield_keyword(other.sti_yield_keyword),
	  sti_semicolon(other.sti_semicolon) {
	{
		auto result = context.push_task(return_value.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		return_value = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API YieldStmtNode::~YieldStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(YieldStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(IfStmtNode);

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

SLKC_API IfStmtNode::IfStmtNode(Global *global)
	: StmtNode(StmtKind::If, global) {
}

SLKC_API IfStmtNode::IfStmtNode(const IfStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  sti_if_keyword(other.sti_if_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis) {
	{
		auto result = context.push_task(condition.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		condition = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(true_branch.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		true_branch = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(false_branch.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		false_branch = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API IfStmtNode::~IfStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(IfStmtNode);

SLKC_API peff::Result<SwitchStmtBranch, DuplicationError> SwitchStmtBranch::duplicate(DuplicationContext &context) const noexcept {
	SwitchStmtBranch branch;

	{
		auto result = context.push_task(pattern.get_index());
		if (!result)
			return DuplicationError::OutOfMemory;
		branch.pattern = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
	{
		auto result = context.push_task(body.get_index());
		if (!result)
			return DuplicationError::OutOfMemory;
		branch.body = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
	branch.sti_case_keyword = sti_case_keyword;
	branch.sti_default_keyword = sti_default_keyword;

	return branch;
}

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

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(SwitchStmtNode);

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

SLKC_API SwitchStmtNode::SwitchStmtNode(Global *global)
	: StmtNode(StmtKind::Switch, global),
	  branches(global->get_allocator()) {
}

SLKC_API SwitchStmtNode::SwitchStmtNode(const SwitchStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  branches(context.get_global()->get_allocator()),
	  sti_switch_keyword(other.sti_switch_keyword),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	{
		auto result = context.push_task(other.condition.get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		condition = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}

	if (!branches.resize(other.branches.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < branches.size(); i++) {
		auto result = other.branches[i].do_duplicate(context);
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		branches[i] = std::move(result).value();
	}
}

SLKC_API SwitchStmtNode::~SwitchStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(SwitchStmtNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(BlockStmtNode);

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

SLKC_API BlockStmtNode::BlockStmtNode(Global *global)
	: StmtNode(StmtKind::Block, global),
	  inner_stmts(global->get_allocator()) {
}

SLKC_API BlockStmtNode::BlockStmtNode(const BlockStmtNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out)
	: StmtNode(other, context, node_index),
	  inner_stmts(context.get_global()->get_allocator()) {
	if (!inner_stmts.resize(other.inner_stmts.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < inner_stmts.size(); i++) {
		auto result = context.push_task(other.inner_stmts[i].get_index());
		if (!result) {
			error_out = DuplicationError::OutOfMemory;
			return;
		}
		inner_stmts[i] = AstNodePtr<StmtNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API BlockStmtNode::~BlockStmtNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BlockStmtNode);
