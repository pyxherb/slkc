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
		case TokenId::LParenthese: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::Group);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(lhs, nullptr, -10)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthese));
			break;
		}
		case TokenId::NewKeyword: {
			lhs->exdata = ExprRGNodeExData(RGExprKind::New);

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(lhs));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(lhs, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::LParenthese));

			RGNodePin args;

			if (!(args = make_rg_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(lhs, args);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_args(args, TokenId::RParenthese, TokenId::Comma)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(lhs, TokenId::RParenthese));
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
		default:
			break;
	}

	co_return peff::NULLOPT;
}
