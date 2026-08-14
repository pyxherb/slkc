#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_generic_constraint(GenericConstraint &constraint_out) {
	GenericConstraint constraint(get_global());

	peff::Option<SyntaxError> syntax_error;

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_inherited_type_slot(constraint.inherited_type, constraint.sti_inherit_left_parenthesis, constraint.sti_inherit_right_parenthesis));
	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_implement_list(constraint.implemented_types, constraint.sti_implement_colon, constraint.sti_implement_item_separator));
	constraint_out = std::move(constraint);

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_generic_params(
	peff::DynArray<NodePtr<GenericParamNode>> &generic_params_out,
	peff::DynArray<TokenIndex> &idx_comma_tokens_out,
	TokenIndex &l_angle_bracket_index_out,
	TokenIndex &r_angle_bracket_index_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_angle_bracket_token = peek_token();

	l_angle_bracket_index_out = l_angle_bracket_token->index;

	if (l_angle_bracket_token->token_id == TokenId::LtOp) {
		next_token();
		while (true) {
			NodePin<GenericParamNode> generic_param_node;

			if (!(generic_param_node = make_node<GenericParamNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			generic_param_node->set_parent(cur_parent.get_index());

			if (!generic_params_out.push_back(NodePtr<GenericParamNode>(generic_param_node)))
				co_return gen_oom_syntax_error();

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));
			;

			peff::Deferred set_token_range_guard([this, name_token, &generic_param_node]() noexcept {
				if (generic_param_node) {
					generic_param_node->set_token_range(TokenRange{ parse_context.mod, name_token->index, parse_context.idx_prev_token });
				}
			});

			if (!generic_param_node->set_name(name_token->source_text))
				co_return gen_oom_syntax_error();

			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_constraint(generic_param_node->generic_constraint));

			if (peek_token()->token_id != TokenId::Comma) {
				break;
			}

			Token *comma_token = next_token();

			if (!idx_comma_tokens_out.push_back(+comma_token->index))
				co_return gen_oom_syntax_error();
		}

		Token *r_angle_bracket_token;

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_angle_bracket_token = peek_token()), TokenId::GtOp));

		next_token();

		r_angle_bracket_index_out = r_angle_bracket_token->index;
	}

	co_return peff::NULLOPT;
}
