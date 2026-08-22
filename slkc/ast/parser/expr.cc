#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_expr(RGNodePin parent, RGNodePin *node_pin_out, int precedence) {
	Token *token;
	RGNodePin lhs, rhs;

	if (!(lhs = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();

	lhs->node_kind = RGNodeKind::Expr;

	peff::ScopeGuard sg([&lhs, &parent, node_pin_out]() noexcept {
		if (parent)
			parent->children.back() = lhs;
		if (node_pin_out)
			*node_pin_out = lhs;
	});

	if (parent) {
		if (!(parent->push_child({}))) {
			sg.release();
			co_return gen_oom_syntax_error();
		}
	} else
		sg.release();

	switch ((token = peek_token())->token_id) {
		case TokenId::ThisKeyword:
		case TokenId::ScopeOp:
		case TokenId::Id: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::IdRef);

			RGNodePin inner_id_ref;
			if (!(inner_id_ref = make_rg_node(get_global())))
				co_return gen_oom_syntax_error();
			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, inner_id_ref);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(inner_id_ref, true)(this));

			break;
		}
		case TokenId::LParenthesis: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::Group);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, -10)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
			break;
		}
		case TokenId::NewKeyword: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::New);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(lhs, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LParenthesis));

			RGNodePin args;

			if (!(args = make_rg_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(args, TokenId::RParenthesis, TokenId::Comma)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
			break;
		}
		case TokenId::I8Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::I8Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I16Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::I16Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I32Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::I32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::I64Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::I64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U8Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::U8Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U16Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::U16Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U32Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::U32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::U64Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::U64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::F32Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::F32Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::F64Literal:
			lhs->exdata = ExprRGNodeExData(RGExprKind::F64Literal);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::StringLiteral:
			lhs->exdata = ExprRGNodeExData(RGExprKind::StringLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::TrueKeyword:
			lhs->exdata = ExprRGNodeExData(RGExprKind::BoolLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::FalseKeyword:
			lhs->exdata = ExprRGNodeExData(RGExprKind::BoolLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::NullKeyword:
			lhs->exdata = ExprRGNodeExData(RGExprKind::NullLiteral);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::LBrace:
			lhs->exdata = ExprRGNodeExData(RGExprKind::InitializerList);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LBrace));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(lhs, TokenId::RBrace, TokenId::Comma)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RBrace));
			break;
		case TokenId::VarArg:
			lhs->exdata = ExprRGNodeExData(RGUnaryExprOp::Unpacking);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::SubOp:
			lhs->exdata = ExprRGNodeExData(RGUnaryExprOp::Neg);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::AddOp:
			lhs->exdata = ExprRGNodeExData(RGUnaryExprOp::Move);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::NotOp:
			lhs->exdata = ExprRGNodeExData(RGUnaryExprOp::Not);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::LNotOp:
			lhs->exdata = ExprRGNodeExData(RGUnaryExprOp::LNot);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));
			break;
		case TokenId::MatchKeyword: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::Match);
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LParenthesis));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LBrace));

			while (true) {
				RGNodePin case_node;

				if (!(case_node = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, case_node);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node, TokenId::CaseKeyword));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(case_node, nullptr, 0)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node, TokenId::Colon));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(case_node, nullptr, 0)(this));

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

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RBrace));
			break;
		}
		default:
			collect_and_next_token(lhs);
			co_return SyntaxError(
				TokenRange{ module_node, token->index },
				SyntaxErrorKind::ExpectingExpr);
	}

	while (true) {
		switch ((token = peek_token())->token_id) {
			case TokenId::LParenthesis: {
				if (precedence > 140)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGExprKind::Call);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LParenthesis));

				RGNodePin args;

				if (!(args = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(args, TokenId::RParenthesis, TokenId::Comma)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthesis));
				break;
			}
			case TokenId::LBracket: {
				if (precedence > 140)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGExprKind::Subscript);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LBracket));

				RGNodePin args;

				if (!(args = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_subscript_args(args)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RBracket));
				break;
			}
			case TokenId::Dot: {
				if (precedence > 140)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGExprKind::HeadedIdRef);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				RGNodePin inner_id_ref;
				if (!(inner_id_ref = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();
				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, inner_id_ref);

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(inner_id_ref, true)(this));
				break;
			}
			case TokenId::AsKeyword: {
				if (precedence > 130)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGExprKind::Cast);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(lhs, nullptr)(this));
				break;
			}
			case TokenId::MulOp: {
				if (precedence > 120)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::DivOp: {
				if (precedence > 120)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Div);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::ModOp: {
				if (precedence > 120)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Mod);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 121)(this));
				break;
			}
			case TokenId::AddOp: {
				if (precedence > 110)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 111)(this));
				break;
			}
			case TokenId::SubOp: {
				if (precedence > 120)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Sub);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 111)(this));
				break;
			}
			case TokenId::ShlOp: {
				if (precedence > 100)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 101)(this));
				break;
			}
			case TokenId::ShrOp: {
				if (precedence > 100)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Mul);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 101)(this));
				break;
			}
			case TokenId::CmpOp: {
				if (precedence > 90)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Cmp);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 91)(this));
				break;
			}
			case TokenId::GtOp: {
				if (precedence > 80)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Gt);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::GtEqOp: {
				if (precedence > 80)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::GtEq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::LtOp: {
				if (precedence > 80)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Lt);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::LtEqOp: {
				if (precedence > 80)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::LtEq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 81)(this));
				break;
			}
			case TokenId::EqOp: {
				if (precedence > 70)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Eq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::NeqOp: {
				if (precedence > 70)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Neq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::StrictEqOp: {
				if (precedence > 70)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Eq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::StrictNeqOp: {
				if (precedence > 70)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Neq);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 71)(this));
				break;
			}
			case TokenId::AndOp: {
				if (precedence > 60)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::And);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 61)(this));
				break;
			}
			case TokenId::XorOp: {
				if (precedence > 50)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Xor);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 51)(this));
				break;
			}
			case TokenId::OrOp: {
				if (precedence > 40)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Or);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 41)(this));
				break;
			}
			case TokenId::LAndOp: {
				if (precedence > 30)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::LAnd);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 31)(this));
				break;
			}
			case TokenId::LOrOp: {
				if (precedence > 20)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::LOr);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 21)(this));
				break;
			}
			case TokenId::Question: {
				if (precedence > 10)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGExprKind::Ternary);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 10)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::Colon));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 10)(this));
				break;
			}
			case TokenId::AssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::Assign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::AddAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::AddAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::SubAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::SubAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::MulAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::MulAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::DivAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::DivAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ModAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::ModAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::AndAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::AndAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::OrAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::OrAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::XorAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::XorAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ShlAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::ShlAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			case TokenId::ShrAssignOp: {
				if (precedence > 1)
					goto end;

				RGNodePin old_lhs = std::move(lhs);
				if (!(lhs = make_rg_node(get_global())))
					co_return gen_oom_syntax_error();

				lhs->node_kind = RGNodeKind::Expr;

				lhs->exdata = ExprRGNodeExData(RGBinaryExprOp::ShrAssign);

				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, old_lhs);

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, 0)(this));
				break;
			}
			default:
				goto end;
		}
	}

end:
	co_return peff::NULLOPT;
}
