#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_params(
	peff::DynArray<BindingEntry> &params_out,
	bool &var_arg_out,
	peff::DynArray<TokenIndex> &idx_comma_tokens_out,
	TokenIndex &l_angle_bracket_index_out,
	TokenIndex &r_angle_bracket_index_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *l_parenthese_token = peek_token();

	l_angle_bracket_index_out = l_parenthese_token->index;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese));

	next_token();

	while (true) {
		if (TokenId next_token_id = peek_token()->token_id; (next_token_id == TokenId::RParenthese) || (next_token_id == TokenId::VarArg)) {
			break;
		}

		BindingEntry param_node;

		Token *name_token;

		SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((name_token = peek_token()), TokenId::Id));

		if (!(param_node.name = GlobalSharedStringRef(get_global()->register_shared_string(name_token->source_text))))
			co_return gen_oom_syntax_error();

		next_token();

		if (peek_token()->token_id == TokenId::Colon) {
			Token *colon_token = next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(param_node.type));
		}

		if (!params_out.push_back(std::move(param_node)))
			co_return gen_oom_syntax_error();

		if (peek_token()->token_id != TokenId::Comma) {
			break;
		}

		Token *comma_token = next_token();

		if (!idx_comma_tokens_out.push_back(+comma_token->index))
			co_return gen_oom_syntax_error();
	}

	Token *var_arg_token;
	if ((var_arg_token = peek_token())->token_id == TokenId::VarArg) {
		next_token();
		var_arg_out = true;
	}

	Token *r_parenthese_token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese));

	next_token();

	r_angle_bracket_index_out = r_parenthese_token->index;

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_fn(NodePin<FnOverloadingNode> &fn_node_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *fn_token;
	Token *lvalue_marker_token = nullptr;

	peff::String name(get_global()->get_allocator());

	if (!(fn_node_out = make_node<FnOverloadingNode>(get_global())))
		co_return gen_oom_syntax_error();

	if (!(fn_node_out->alloc_scope()))
		co_return gen_oom_syntax_error();

	switch ((fn_token = peek_token())->token_id) {
		case TokenId::FnKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Regular;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_name(name));
			break;
		}
		case TokenId::AsyncKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Coroutine;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_name(name));
			break;
		}
		case TokenId::OperatorKeyword: {
			next_token();

			fn_node_out->overloading_kind = FnOverloadingKind::Regular;

			std::string_view operator_name;
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_operator_name(operator_name));

			if (!name.build(operator_name)) {
				co_return gen_oom_syntax_error();
			}
			break;
		}
		default:
			co_return SyntaxError(TokenRange{ parse_context.mod, fn_token->index }, SyntaxErrorKind::UnexpectedToken);
	}

	switch (cur_parent->get_ast_node_type()) {
		case NodeType::Interface:
			fn_node_out->overloading_flags = OVERLOADING_FLAG_VIRTUAL;
			break;
		default:
			break;
	}

	NodePin<MemberNode> prev_parent = cur_parent;
	peff::ScopeGuard restore_parent_guard([this, prev_parent]() noexcept {
		cur_parent = prev_parent;
	});
	cur_parent = fn_node_out.cast_to<MemberNode>();

	peff::Deferred set_token_range_guard([this, fn_token, fn_node_out]() noexcept {
		fn_node_out->set_token_range(TokenRange{ parse_context.mod, fn_token->index, parse_context.idx_prev_token });
	});

	fn_node_out->set_name(name);
	name.clear();

	auto scope = fn_node_out->get_scope();
	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(scope->generic_params, fn_node_out->idx_generic_param_comma_tokens, fn_node_out->sti_generic_left_angle, fn_node_out->sti_generic_right_angle));
	for (size_t i = 0; i < scope->generic_params.size(); ++i) {
		auto gp = scope->generic_params.at(i);

		auto pinned_gp = gp.pin();
		if (!pinned_gp) {
			switch (pinned_gp.get_fail_reason()) {
				case PinFailReason::OutOfMemory:
					co_return gen_oom_syntax_error();
					break;
				case PinFailReason::IOError:
					co_return gen_pinning_io_error();
					break;
				default:
					co_return SyntaxError(TokenRange{ parse_context.mod, parse_context.idx_current_token }, SyntaxErrorKind::UnexpectedToken);
					break;
			}
		}

		if (scope->generic_params_index.contains(pinned_gp->get_name())) {
			peff::String s(get_global()->get_allocator());

			if (!s.build(pinned_gp->get_name())) {
				co_return gen_oom_syntax_error();
			}

			ConflictingDefinitionsErrorExData ex_data(std::move(s));

			co_return SyntaxError(pinned_gp->get_token_range(), std::move(ex_data));
		}
		if (auto result = scope->index_generic_param(+i); result != ScopeMemberOpResult::Success)
			co_return scope_member_op_result_to_syntax_error(result);
	}

	bool has_var_arg = false;
	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_params(fn_node_out->params, has_var_arg, fn_node_out->idx_param_comma_tokens, fn_node_out->sti_left_parenthesis, fn_node_out->sti_right_parenthesis));
	if (has_var_arg) {
		fn_node_out->overloading_flags |= OVERLOADING_FLAG_VARARG;
	}
	// TODO: Index the parameters.
	/*for (size_t i = 0; i < fn_node_out->params.size(); ++i) {
		BindingEntry &cur_param = fn_node_out->params.at(i);
		if (fn_node_out->param_indices.contains(cur_param.name)) {
			peff::String s(get_global()->get_allocator());

			if (!s.build(cur_param.name)) {
				co_return gen_oom_syntax_error();
			}

			ConflictingDefinitionsErrorExData ex_data(std::move(s));

			if (!syntax_errors.push_back(SyntaxError(cur_param.token_range, std::move(ex_data))))
				co_return gen_oom_syntax_error();
		}

		if (!fn_node_out->param_indices.insert(cur_param.name, +i)) {
			co_return gen_oom_syntax_error();
		}
	}*/

	Token *virtual_token;
	if ((virtual_token = peek_token())->token_id == TokenId::VirtualKeyword) {
		fn_node_out->overloading_flags |= OVERLOADING_FLAG_VIRTUAL;
		next_token();
	}

	Token *override_token;
	if ((override_token = peek_token())->token_id == TokenId::OverrideKeyword) {
		next_token();

		fn_node_out->overloading_flags |= OVERLOADING_FLAG_OVERRIDE;

		Token *lookahead_token = peek_token();
		switch (lookahead_token->token_id) {
			case TokenId::ReturnTypeOp:
			case TokenId::Semicolon:
			case TokenId::LBrace:
				break;
			default: {
				TypeName overriden_type;
				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(overriden_type));
				fn_node_out->overriden_type = overriden_type;
				break;
			}
		}
	}

	Token *return_type_token;
	if ((return_type_token = peek_token())->token_id == TokenId::ReturnTypeOp) {
		next_token();
		SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(fn_node_out->return_type));
	}

	Token *body_token = peek_token();

	switch (body_token->token_id) {
		case TokenId::Semicolon: {
			next_token();

			break;
		}
		case TokenId::LBrace: {
			next_token();

			NodePin<BlockStmtNode> body;
			NodePtr<StmtNode> cur_stmt;

			if (!(body = make_node<BlockStmtNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}
			fn_node_out->body = body;

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
					if (!body->inner_stmts.push_back(std::move(cur_stmt))) {
						co_return gen_oom_syntax_error();
					}
				}
			}

			Token *r_brace_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

			next_token();
			break;
		}
		default:
			co_return SyntaxError(
				TokenRange{ parse_context.mod, body_token->index },
				SyntaxErrorKind::UnexpectedToken);
	}

	co_return peff::NULLOPT;
}