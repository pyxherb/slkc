#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_var_binding(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin binding_node_out;
	if (!(binding_node_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	binding_node_out->node_kind = RGNodeKind::Stmt;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &binding_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = binding_node_out;
	});

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(binding_node_out, TokenId::Id));

	Token *token;
	if ((token = peek_token())->token_id == TokenId::Colon) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(binding_node_out));

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(binding_node_out, nullptr)(this));
	}

	if ((token = peek_token())->token_id == TokenId::AssignOp) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(binding_node_out));

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(binding_node_out, nullptr, 0)(this));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_var_binding_list(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin binding_list_node_out;
	if (!(binding_list_node_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	binding_list_node_out->node_kind = RGNodeKind::Stmt;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &binding_list_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = binding_list_node_out;
	});

	binding_list_node_out->node_kind = RGNodeKind::Args;

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, binding_list_node_out);

	Token *token;

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding(binding_list_node_out, nullptr)(this));

		if ((token = peek_token())->token_id != TokenId::Comma)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(binding_list_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_stmt(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin stmt_node_out;
	if (!(stmt_node_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	stmt_node_out->node_kind = RGNodeKind::Stmt;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &stmt_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = stmt_node_out;
	});

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, stmt_node_out);

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::IfKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::IfStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(stmt_node_out, nullptr)(this));

			break;
		}
		case TokenId::ForKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::ForStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(stmt_node_out, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			if ((token = peek_token())->token_id != TokenId::RParenthesis) {
				while (true) {
					SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

					if ((token = peek_token())->token_id != TokenId::Comma)
						break;

					SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));
				}
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(stmt_node_out, nullptr)(this));

			break;
		}
		case TokenId::WhileKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::WhileStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(stmt_node_out, nullptr)(this));

			break;
		}
		case TokenId::DoKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::DoWhileStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(stmt_node_out, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			break;
		}
		case TokenId::LetKeyword:
		case TokenId::VarKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::LocalVarStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(stmt_node_out, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));
			break;
		}
		case TokenId::BreakKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::BreakStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			break;
		}
		case TokenId::ContinueKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::ContinueStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			break;
		}
		case TokenId::ReturnKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::ReturnStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			break;
		}
		case TokenId::YieldKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::YieldStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::Semicolon));

			break;
		}
		case TokenId::SwitchKeyword: {
			stmt_node_out->exdata = StmtRGNodeExData{ RGStmtKind::SwitchStmt };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LBrace));

			while (true) {
				RGNodePin case_node;

				if (!(case_node = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, case_node);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node, TokenId::CaseKeyword));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::LParenthesis));
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(case_node, nullptr, 0)(this));
				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RParenthesis));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(case_node, nullptr)(this));

				if ((token = peek_token())->token_id == TokenId::RBrace)
					break;

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RBrace));

			break;
		}
		case TokenId::LBrace: {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));

			while (true) {
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(stmt_node_out, nullptr)(this));

				if ((token = peek_token())->token_id == TokenId::RBrace)
					break;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(stmt_node_out, TokenId::RBrace));

			break;
		}
		default:
			while (true) {
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(stmt_node_out, nullptr, 0)(this));

				if ((token = peek_token())->token_id != TokenId::Comma)
					break;

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(stmt_node_out));
			}
			break;
	}

	co_return peff::NULLOPT;
}
