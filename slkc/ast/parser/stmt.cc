#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_var_defs(peff::DynArray<BindingEntry> &var_def_entries) {
	Token *current_token;
	peff::Option<SyntaxError> syntax_error;

	for (;;) {
		/*peff::DynArray<NodePtr<AttributeNode>> attributes(get_global()->get_allocator());

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_attributes(attributes));

		if ((syntax_error = expect_token((current_token = next_token()), TokenId::Id))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
			if (!syntax_errors.push_back(SyntaxError({ parse_context.mod, current_token->index }, SyntaxErrorKind::ExpectingId)))
				co_return gen_oom_syntax_error();
		}*/

		BindingEntry entry;

		if (!var_def_entries.push_back(std::move(entry))) {
			co_return gen_oom_syntax_error();
		}

		// entry.token_range = current_token->index;
		if (!(entry.name = GlobalSharedStringRef(get_global()->register_shared_string(current_token->source_text))))
			co_return gen_oom_syntax_error();

		if ((current_token = peek_token())->token_id == TokenId::Colon) {
			next_token();

			if ((syntax_error = co_await (parse_type_name(entry.type)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_oom_syntax_error();
				syntax_error.reset();
			}
		}

		if ((current_token = peek_token())->token_id == TokenId::AssignOp) {
			next_token();

			if ((syntax_error = co_await (parse_expr(0, entry.initial_value)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_oom_syntax_error();
				syntax_error.reset();
			}
		}

		/*
		entry.attributes = std::move(attributes);

		if ((current_token = peek_token())->token_id != TokenId::Comma) {
			break;
		}*/

		next_token();
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_if_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<IfStmtNode> if_stmt;

	if (!(if_stmt = make_node<IfStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = if_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(0, if_stmt->condition)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}
	}

	Token *r_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(if_stmt->true_branch));

	Token *else_token = peek_token();

	if (else_token->token_id == TokenId::ElseKeyword) {
		next_token();

		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(if_stmt->false_branch));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_for_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<ForStmtNode> for_stmt;

	if (!(for_stmt = make_node<ForStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = for_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *var_def_separator_token;
	Token *cond_separator_token;
	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((var_def_separator_token = peek_token())->token_id != TokenId::Semicolon) {
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(for_stmt->loop_vars));

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((var_def_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((cond_separator_token = peek_token())->token_id != TokenId::Semicolon) {
			if ((syntax_error = co_await (parse_expr(0, for_stmt->condition_expr)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((cond_separator_token = peek_token()), TokenId::Semicolon));
			next_token();
		} else {
			next_token();
		}

		if ((r_parenthese_token = peek_token())->token_id != TokenId::RParenthese) {
			if ((syntax_error = co_await (parse_expr(-10, for_stmt->step_expr)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));
			next_token();
		} else {
			next_token();
		}
	}

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(for_stmt->body));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_while_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<WhileStmtNode> while_stmt;

	if (!(while_stmt = make_node<WhileStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(0, while_stmt->condition_expr)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(while_stmt->body));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_do_while_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<DoWhileStmtNode> while_stmt;

	if (!(while_stmt = make_node<DoWhileStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = while_stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_stmt(while_stmt->body));

	Token *while_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(while_token, TokenId::WhileKeyword));

	next_token();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	Token *r_parenthese_token;
	{
		static TokenId skipping_terminative_token[] = {
			TokenId::RParenthese,
			TokenId::Semicolon,
			TokenId::RBrace
		};

		if ((syntax_error = co_await (parse_expr(0, while_stmt->condition_expr)(this)))) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
			co_return syntax_error;
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

		next_token();
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_let_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<VarDefStmtNode> stmt;

	if (!(stmt = make_node<VarDefStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(stmt->bindings));

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_break_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePtr<BreakStmtNode> stmt;

	if (!(stmt = make_node<BreakStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_continue_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePtr<ContinueStmtNode> stmt;

	if (!(stmt = make_node<ContinueStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *semicolon_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((semicolon_token = peek_token()), TokenId::Semicolon));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_return_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<ReturnStmtNode> stmt;

	if (!(stmt = make_node<ReturnStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	static TokenId skipping_terminative_token[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peek_token()->token_id) {
		case TokenId::Semicolon:
			next_token();
			break;
		default:
			if ((syntax_error = co_await (parse_expr(0, stmt->return_value)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_yield_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<YieldStmtNode> stmt;

	if (!(stmt = make_node<YieldStmtNode>(
			  get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	static TokenId skipping_terminative_token[] = {
		TokenId::RParenthese,
		TokenId::Semicolon,
		TokenId::RBrace
	};

	switch (peek_token()->token_id) {
		case TokenId::Semicolon:
			next_token();
			break;
		default:
			if ((syntax_error = co_await (parse_expr(0, stmt->return_value)(this)))) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(lookahead_until(std::size(skipping_terminative_token), skipping_terminative_token));
				co_return syntax_error;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

			next_token();
			break;
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_block_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<BlockStmtNode> stmt;
	NodePtr<StmtNode> cur_stmt;

	if (!(stmt = make_node<BlockStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	while (true) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = co_await (parse_stmt(cur_stmt)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
		}

		if (cur_stmt) {
			if (!stmt->inner_stmts.push_back(std::move(cur_stmt))) {
				co_return gen_oom_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	co_return peff::NULLOPT;
}

/*SLKC_API ParseCoroutine Parser::parse_switch_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePin<SwitchStmtNode> stmt;

	if (!(stmt = make_node<SwitchStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	Token *l_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_parenthese_token, TokenId::LParenthese));

	next_token();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(0, stmt->condition));

	Token *r_parenthese_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(r_parenthese_token, TokenId::RParenthese));

	next_token();

	Token *l_brace_token = peek_token();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(l_brace_token, TokenId::LBrace));

	next_token();

	NodePtr<StmtNode> cur_stmt;

	while (true) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token()));

		if (peek_token()->token_id == TokenId::RBrace) {
			break;
		}

		if ((syntax_error = co_await (parse_stmt(cur_stmt)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
		}

		if (cur_stmt) {
			// We detect and push case labels in advance to deal with them easier.
			if (cur_stmt->get_stmt_kind() == StmtKind::CaseLabel) {
				if (!stmt->case_offsets.push_back(stmt->body.size()))
					co_return gen_oom_syntax_error();
			}

			if (!stmt->body.push_back(std::move(cur_stmt))) {
				co_return gen_oom_syntax_error();
			}
		}
	}

	Token *r_brace_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_brace_token = peek_token()), TokenId::RBrace));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_case_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_node<CaseLabelStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(0, stmt->condition));

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_default_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	next_token();

	NodePtr<CaseLabelStmtNode> stmt;

	if (!(stmt = make_node<CaseLabelStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Colon));

	next_token();

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_expr_stmt(NodePtr<StmtNode> &stmt_out) {
	peff::Option<SyntaxError> syntax_error;

	NodePtr<ExprNode> cur_expr;

	NodePtr<ExprStmtNode> stmt;

	if (!(stmt = make_node<ExprStmtNode>(get_global()))) {
		co_return gen_oom_syntax_error();
	}

	stmt_out = stmt.cast_to<StmtNode>();

	if ((syntax_error = co_await (parse_expr(-10, stmt->expr)(this)))) {
		if (!syntax_errors.push_back(std::move(syntax_error.value())))
			co_return gen_oom_syntax_error();
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), TokenId::Semicolon));

	next_token();

	co_return peff::NULLOPT;
}*/

SLKC_API ParseCoroutine Parser::parse_stmt(NodePtr<StmtNode> &stmt_out) {
	Token *prefix_token;

	peff::Option<SyntaxError> syntax_error;

	if ((syntax_error = expect_token((prefix_token = peek_token()))))
		goto gen_bad_stmt;

	{
		switch (prefix_token->token_id) {
			case TokenId::IfKeyword:
				if ((syntax_error = co_await (parse_if_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ForKeyword:
				if ((syntax_error = co_await (parse_for_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::WhileKeyword:
				if ((syntax_error = co_await (parse_while_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::DoKeyword:
				if ((syntax_error = co_await (parse_do_while_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::LetKeyword:
				if ((syntax_error = co_await (parse_let_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::BreakKeyword:
				if ((syntax_error = co_await (parse_break_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ContinueKeyword:
				if ((syntax_error = co_await (parse_continue_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::ReturnKeyword:
				if ((syntax_error = co_await (parse_return_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::YieldKeyword:
				if ((syntax_error = co_await (parse_yield_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::CaseKeyword:
				if ((syntax_error = co_await (parse_case_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::DefaultKeyword:
				if ((syntax_error = co_await (parse_default_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::SwitchKeyword:
				if ((syntax_error = co_await (parse_switch_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			case TokenId::LBrace:
				if ((syntax_error = co_await (parse_block_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
			default:
				if ((syntax_error = co_await (parse_expr_stmt(stmt_out)(this))))
					goto gen_bad_stmt;
				break;
		}
	}

	{
		auto pinned_m = stmt_out.pin();
		if (!pinned_m) {
			switch (pinned_m.get_fail_reason()) {
				case PinFailReason::OutOfMemory:
					co_return gen_oom_syntax_error();
					break;
				case PinFailReason::IOError:
					co_return gen_pinning_io_error();
					break;
				default:
					co_return SyntaxError(TokenRange{ parse_context.mod, prefix_token->index }, SyntaxErrorKind::UnexpectedToken);
					break;
			}
		}
		pinned_m->set_token_range(TokenRange{ parse_context.mod, prefix_token->index, parse_context.idx_current_token });
	}

	co_return peff::NULLOPT;

gen_bad_stmt:
	auto pinned_stmt = stmt_out.pin();
	if (!pinned_stmt) {
		switch (pinned_stmt.get_fail_reason()) {
			case PinFailReason::OutOfMemory:
				co_return gen_oom_syntax_error();
				break;
			case PinFailReason::IOError:
				co_return gen_pinning_io_error();
				break;
			default:
				co_return SyntaxError(TokenRange{ parse_context.mod, prefix_token->index }, SyntaxErrorKind::UnexpectedToken);
				break;
		}
	}
	pinned_stmt->set_token_range(TokenRange{ parse_context.mod, prefix_token->index, parse_context.idx_current_token });
	co_return syntax_error;
}
