#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_expr(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out, int precedence) {
	Token *token;
	GreenNodePin lhs;

	if (!(lhs = make_green_node(get_global())))
		co_return gen_oom_syntax_error();

	lhs->node_kind = GreenNodeKind::Expr;

	peff::ScopeGuard sg([&lhs, &parent, node_pin_out]() noexcept {
		parent->children.back() = lhs;
		if (node_pin_out)
			*node_pin_out = lhs;
	});

	if (!(parent->push_child({}))) {
		sg.release();
		co_return gen_oom_syntax_error();
	}

	switch ((token = peek_token())->token_id) {
		case TokenId::ThisKeyword:
		case TokenId::ScopeOp:
		case TokenId::Id: {
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::IdRef);

			GreenNodePin inner_id_ref;
			if (!(inner_id_ref = make_green_node(get_global())))
				co_return gen_oom_syntax_error();
			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, inner_id_ref);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(allocator, inner_id_ref, true)(this));

			break;
		}
		case TokenId::LParenthesis: {
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Group);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, -10)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
			break;
		}
		case TokenId::NewKeyword: {
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::New);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, lhs, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LParenthesis));

			GreenNodePin args;

			if (!(args = make_green_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(allocator, args, TokenId::RParenthesis, TokenId::Comma)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
			break;
		}
		case TokenId::I8Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::I8Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I16Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::I16Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I32Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::I32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I64Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::I64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U8Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::U8Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U16Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::U16Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U32Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::U32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U64Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::U64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::F32Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::F32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::F64Literal:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::F64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::StringLiteral:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::StringLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::TrueKeyword:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::BoolLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::FalseKeyword:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::BoolLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::NullKeyword:
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::NullLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::LBrace: {
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::InitializerList);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LBrace));

			GreenNodePin args;

			if (!(args = make_green_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(allocator, args, TokenId::RBrace, TokenId::Comma)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RBrace));
			break;
		}
		case TokenId::VarArg:
			lhs->exdata = ExprGreenNodeExData(GreenNodeUnaryExprOp::Unpacking);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::SubOp:
			lhs->exdata = ExprGreenNodeExData(GreenNodeUnaryExprOp::Neg);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::AddOp:
			lhs->exdata = ExprGreenNodeExData(GreenNodeUnaryExprOp::Move);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::NotOp:
			lhs->exdata = ExprGreenNodeExData(GreenNodeUnaryExprOp::Not);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::LNotOp:
			lhs->exdata = ExprGreenNodeExData(GreenNodeUnaryExprOp::LNot);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::MatchKeyword: {
			lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Match);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(lhs, TokenId::LParenthesis));

			SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, co_await parse_expr(allocator, lhs, nullptr, 0)(this));

			SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(lhs, TokenId::RParenthesis));

			if ((token = peek_token())->token_id == TokenId::ReturnTypeOp) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, lhs, nullptr)(this));
			}

			SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(lhs, TokenId::LBrace));

			while (true) {
				GreenNodePin case_node;

				if (!(case_node = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				case_node->node_kind = GreenNodeKind::MatchCase;

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, case_node);

				SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(case_node, TokenId::CaseKeyword));

				SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, co_await parse_expr(allocator, case_node, nullptr, 0)(this));

				SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(case_node, TokenId::Colon));

				SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, co_await parse_expr(allocator, case_node, nullptr, 0)(this));

				if ((token = peek_token())->token_id == TokenId::RBrace)
					break;

				if ((token = peek_token())->token_id != TokenId::Comma) {
					if (!syntax_errors.push_back(
							SyntaxError(
								TokenRange{
									module_node,
									parse_context.idx_current_token },
								ExpectingSingleTokenErrorExData{ TokenId::Comma })))
						co_return gen_oom_syntax_error();
				}

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			}

			SLKC_PUSH_IF_PARSE_ERROR(syntax_errors, collect_and_expect_token(lhs, TokenId::RBrace));
			break;
		}
		case TokenId::BreakKeyword: {
			lhs->exdata = ExprGreenNodeExData{ GreenNodeExprKind::Break };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			break;
		}
		case TokenId::ContinueKeyword: {
			lhs->exdata = ExprGreenNodeExData{ GreenNodeExprKind::Continue };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			if (peek_token()->token_id == TokenId::LParenthesis) {
				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				GreenNodePin args;

				if (!(args = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(allocator, args, TokenId::RParenthesis, TokenId::Comma)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
			}

			break;
		}
		case TokenId::ReturnKeyword: {
			lhs->exdata = ExprGreenNodeExData{ GreenNodeExprKind::Return };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			if ((token = peek_token())->token_id != TokenId::Semicolon)
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));

			break;
		}
		case TokenId::YieldKeyword: {
			lhs->exdata = ExprGreenNodeExData{ GreenNodeExprKind::Yield };

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));

			break;
		}
		default:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			co_return SyntaxError(
				TokenRange{ module_node, token->index },
				SyntaxErrorKind::ExpectingExpr);
	}

	while (true) {
		switch ((token = peek_token())->token_id) {
			case TokenId::LParenthesis: {
				if (precedence > 140)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Call);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				GreenNodePin args;

				if (!(args = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(allocator, args, TokenId::RParenthesis, TokenId::Comma)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
				break;
			}
			case TokenId::LBracket: {
				if (precedence > 140)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Subscript);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				GreenNodePin args;

				if (!(args = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_subscript_args(allocator, args)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RBracket));
				break;
			}
			case TokenId::Dot: {
				if (precedence > 140)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::HeadedIdRef);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				GreenNodePin inner_id_ref;
				if (!(inner_id_ref = make_green_node(get_global())))
					co_return gen_oom_syntax_error();
				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, inner_id_ref);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(allocator, inner_id_ref, true)(this));
				break;
			}
			case TokenId::AsKeyword: {
				if (precedence > 130)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Cast);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, lhs, nullptr)(this));
				break;
			}
			case TokenId::MulOp: {
				if (precedence > 120)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::DivOp: {
				if (precedence > 120)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Div);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::ModOp: {
				if (precedence > 120)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Mod);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::AddOp: {
				if (precedence > 110)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 111)(this));
				break;
			}
			case TokenId::SubOp: {
				if (precedence > 120)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Sub);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 111)(this));
				break;
			}
			case TokenId::ShlOp: {
				if (precedence > 100)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 101)(this));
				break;
			}
			case TokenId::ShrOp: {
				if (precedence > 100)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 101)(this));
				break;
			}
			case TokenId::CmpOp: {
				if (precedence > 90)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Cmp);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 91)(this));
				break;
			}
			case TokenId::GtOp: {
				if (precedence > 80)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Gt);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::GtEqOp: {
				if (precedence > 80)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::GtEq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::LtOp: {
				if (precedence > 80)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Lt);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::LtEqOp: {
				if (precedence > 80)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::LtEq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::EqOp: {
				if (precedence > 70)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Eq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::NeqOp: {
				if (precedence > 70)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Neq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::StrictEqOp: {
				if (precedence > 70)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Eq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::StrictNeqOp: {
				if (precedence > 70)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Neq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::AndOp: {
				if (precedence > 60)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::And);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 61)(this));
				break;
			}
			case TokenId::XorOp: {
				if (precedence > 50)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Xor);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 51)(this));
				break;
			}
			case TokenId::OrOp: {
				if (precedence > 40)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Or);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 41)(this));
				break;
			}
			case TokenId::LAndOp: {
				if (precedence > 30)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::LAnd);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 31)(this));
				break;
			}
			case TokenId::LOrOp: {
				if (precedence > 20)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::LOr);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 21)(this));
				break;
			}
			case TokenId::Question: {
				if (precedence > 10)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeExprKind::Ternary);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 10)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::Colon));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 10)(this));
				break;
			}
			case TokenId::AssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::Assign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::AddAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::AddAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::SubAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::SubAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::MulAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::MulAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::DivAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::DivAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ModAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::ModAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::AndAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::AndAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::OrAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::OrAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::XorAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::XorAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ShlAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::ShlAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ShrAssignOp: {
				if (precedence > 1)
					goto end;

				GreenNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_green_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = GreenNodeKind::Expr;

				lhs->exdata = ExprGreenNodeExData(GreenNodeBinaryExprOp::ShrAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, lhs, nullptr, 0)(this));
				break;
			}
			default:
				goto end;
		}
	}

end:
	co_return peff::NULLOPT;
}
