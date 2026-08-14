#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API ParseCoroutine Parser::parse_generic_arg(TypeName &arg_out) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(arg_out));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_type_name(TypeName &type_name_out, bool with_circumfixes) {
	peff::Option<SyntaxError> syntax_error;
	Token *t = peek_token();

	switch (t->token_id) {
		case TokenId::VoidTypeName:
			type_name_out = TypeName(TypeNameKind::Void, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::I8TypeName:
			type_name_out = TypeName(TypeNameKind::I8, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::I16TypeName:
			type_name_out = TypeName(TypeNameKind::I16, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::I32TypeName:
			type_name_out = TypeName(TypeNameKind::I32, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::I64TypeName:
			type_name_out = TypeName(TypeNameKind::I64, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::U8TypeName:
			type_name_out = TypeName(TypeNameKind::U8, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::U16TypeName:
			type_name_out = TypeName(TypeNameKind::U16, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::U32TypeName:
			type_name_out = TypeName(TypeNameKind::U32, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::U64TypeName:
			type_name_out = TypeName(TypeNameKind::U64, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::ISizeTypeName:
			type_name_out = TypeName(TypeNameKind::ISize, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::USizeTypeName:
			type_name_out = TypeName(TypeNameKind::USize, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::F32TypeName:
			type_name_out = TypeName(TypeNameKind::F32, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::F64TypeName:
			type_name_out = TypeName(TypeNameKind::F64, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::BoolTypeName:
			type_name_out = TypeName(TypeNameKind::Bool, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		case TokenId::StringTypeName:
			type_name_out = TypeName(TypeNameKind::String, {});
			type_name_out.token_range = TokenRange{ parse_context.mod, t->index };
			next_token();
			break;
		/*case TokenId::FnKeyword: {
			NodePtr<FnTypeNameNode> tn;
			if (!(tn = make_node<FnTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(), get_document())))
				co_return gen_oom_syntax_error();
			type_name_out = tn.cast_to<TypeNameNode>();
			tn->token_range = TokenRange{ get_document()->main_module, t->index };
			next_token();

			Token *l_parenthese_token;
			if ((syntax_error = expect_token((l_parenthese_token = peek_token()), TokenId::LParenthese)))
				co_return SyntaxError(TokenRange{ get_document()->main_module, l_parenthese_token->index }, ExpectingSingleTokenErrorExData{ TokenId::LParenthese });

			next_token();

			for (;;) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				TypeName param_type;

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(param_type));

				if (!tn->param_types.push_back(std::move(param_type)))
					co_return gen_oom_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();
			}

			Token *r_parenthese_token;
			if ((syntax_error = expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)))
				co_return SyntaxError(TokenRange{ get_document()->main_module, r_parenthese_token->index }, ExpectingSingleTokenErrorExData{ TokenId::RParenthese });

			next_token();

			if (peek_token()->token_id == TokenId::WithKeyword) {
				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(tn->this_type));
			}

			if (peek_token()->token_id == TokenId::ReturnTypeOp) {
				next_token();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(tn->return_type));
			}

			break;
		}
		case TokenId::LBracket: {
			NodePtr<TupleTypeNameNode> tn;

			if (!(tn = make_node<TupleTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  get_document())))
				co_return gen_oom_syntax_error();

			type_name_out = tn.cast_to<TypeNameNode>();

			Token *l_bracket_token;

			if (auto e = expect_token(l_bracket_token = peek_token(), TokenId::LBracket))
				co_return e;

			tn->idx_lbracket_token = l_bracket_token->index;

			next_token();

			for (;;) {
				if (peek_token()->token_id == TokenId::RParenthese) {
					break;
				}

				TypeName t;

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(t));

				if (!tn->element_types.push_back(std::move(t)))
					co_return gen_oom_syntax_error();

				if (peek_token()->token_id != TokenId::Comma) {
					break;
				}

				Token *comma_token = next_token();

				if (!tn->idx_comma_tokens.push_back(+comma_token->index))
					co_return gen_oom_syntax_error();
			}

			Token *r_bracket_token;

			if (auto e = expect_token(r_bracket_token = peek_token(), TokenId::RBracket))
				co_return e;

			tn->idx_rbracket_token = r_bracket_token->index;

			next_token();

			break;
		}
		case TokenId::SIMDTypeName: {
			NodePtr<SIMDTypeNameNode> tn;

			next_token();

			if (!(tn = make_node<SIMDTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  get_document())))
				co_return gen_oom_syntax_error();

			type_name_out = tn.cast_to<TypeNameNode>();

			Token *l_angle_bracket_token;

			if (auto e = expect_token(l_angle_bracket_token = peek_token(), TokenId::LtOp); e)
				co_return e;

			tn->idx_langle_bracket_token = l_angle_bracket_token->index;

			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(tn->element_type));

			Token *comma_token;

			if (auto e = expect_token(comma_token = peek_token(), TokenId::Comma))
				co_return e;

			tn->idx_comma_token = comma_token->index;

			next_token();

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_expr(140, tn->width));

			Token *r_angle_bracket_token;

			if (auto e = expect_token(r_angle_bracket_token = peek_token(), TokenId::GtOp))
				co_return e;

			tn->idx_rangle_bracket_token = r_angle_bracket_token->index;

			next_token();

			break;
		}*/
		case TokenId::Id: {
			OwnedIdRef id(get_global()->get_allocator());
			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_ref(id, true));

			NodePin<CustomTypeDefNode> tn;

			if (!(tn = make_node<CustomTypeDefNode>(
					  get_global())))
				co_return gen_oom_syntax_error();

			// tn->context_node = to_weak_ptr(cur_parent);

			tn->set_token_range(TokenRange{ parse_context.mod, t->index, parse_context.idx_current_token });
			tn->referred_name = std::move(id);

            type_name_out = TypeName(
                TypeNameKind::Custom,
                tn.cast_to<TypeNameDefNode>()
            );

			break;
		}
		default:
			co_return SyntaxError(TokenRange{ parse_context.mod, t->index }, SyntaxErrorKind::UnexpectedToken);
	}

	if (with_circumfixes) {
		while (true) {
			switch ((t = peek_token())->token_id) {
				case TokenId::FinalKeyword: {
					next_token();

					type_name_out.set_final(true);
					break;
				}
				case TokenId::LocalKeyword: {
					next_token();

					type_name_out.set_local(true);
					break;
				}
				case TokenId::Question: {
					next_token();

					type_name_out.set_nullable(true);
					break;
				}
				/*case TokenId::LBracket: {
					next_token();

					Token *r_bracket_token;
					if ((syntax_error = expect_token((r_bracket_token = peek_token()), TokenId::RBracket)))
						co_return SyntaxError(TokenRange{ get_document()->main_module, r_bracket_token->index }, ExpectingSingleTokenErrorExData{ TokenId::RBracket });

					next_token();

					if (!(type_name_out = make_node<ArrayTypeNameNode>(
							  resource_allocator.get(),
							  resource_allocator.get(),
							  get_document(),
							  type_name_out)
								.cast_to<TypeNameNode>()))
						co_return gen_oom_syntax_error();
					break;
				}*/
				default:
					goto end;
			}
		}
	}

end:
	/*if (with_circumfixes) {
		if ((t = peek_token())->token_id == TokenId::AndOp) {
			next_token();
			if (!(type_name_out = make_node<RefTypeNameNode>(
					  resource_allocator.get(),
					  resource_allocator.get(),
					  get_document(),
					  type_name_out)
						.cast_to<TypeNameNode>()))
				co_return gen_oom_syntax_error();
		}
	}*/

	co_return peff::NULLOPT;
}
