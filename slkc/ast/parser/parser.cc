#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLAKE_API peff::Option<SyntaxError> ParseCoroutine::resume(Parser *parser) {
	if (!coro_handle)
		return parser->gen_oom_syntax_error();

	coro_handle.resume();

	while (parser->parse_coro_scheduler.task_list.size()) {
		auto h = parser->parse_coro_scheduler.task_list.back();
		parser->parse_coro_scheduler.task_list.pop_back();
		if (!h.done())
			h.resume();
		if (coro_handle.promise().result)
			return std::move(coro_handle.promise().result);
	}

	if (coro_handle.promise().result)
		return std::move(coro_handle.promise().result);
	if (!coro_handle.done())
		std::terminate();

	return peff::NULLOPT;
}

SLKC_API ParseCoroutine::Awaitable::Awaitable(
	ParseCoroutine &co,
	Parser *parser,
	ParseCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  parser(parser),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLKC_API bool ParseCoroutine::Awaitable::await_ready() {
	return false;
}

SLKC_API void ParseCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = parser->gen_oom_syntax_error();
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = parser->gen_oom_syntax_error();
		return;
	}
}

SLKC_API peff::Option<SyntaxError> ParseCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result)
			return std::move(handle.promise().result);
		return peff::NULLOPT;
	}
	return parser->gen_oom_syntax_error();
}

SLKC_API ParseCoroutine::Awaitable ParseCoroutine::operator()(Parser *parser) {
	return Awaitable(*this, parser, &parser->parse_coro_scheduler, coro_handle);
}

SLKC_API ParseCoroutineScheduler::ParseCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

SLKC_API Parser::Parser(Global *global, TokenList &&token_list, peff::Alloc *resource_allocator)
	: parse_coro_scheduler(resource_allocator),
	  token_list(std::move(token_list)),
	  syntax_errors(resource_allocator),
	  syntax_warnings(resource_allocator),
	  global(global) {
}

SLKC_API Parser::~Parser() {
}

SLKC_API peff::Option<SyntaxError> Parser::to_next_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	TokenIndex &i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		const auto &current_token = token_list.at(i);
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					return peff::NULLOPT;
				}
				break;
			case TokenId::Whitespace:
				if (keep_whitespace == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					return peff::NULLOPT;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					return peff::NULLOPT;
				}
				break;
			default:
				parse_context.idx_prev_token = parse_context.idx_current_token;
				return peff::NULLOPT;
		}

		SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	}

	return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), i - 1 }, SyntaxErrorKind::ExpectingMoreTokens);
}

/*SLKC_API void Parser::next_token() {
	TokenIndex &i = parse_context.idx_current_token;

	if (i < token_list.size()) {
		const auto &current_token = token_list.at(i);
		current_token->index = i;

		parse_context.idx_prev_token = i;
		++i;
	}
}*/

SLKC_API peff::Option<SyntaxError> Parser::collect_token(const GreenNodePin &parent_node) {
	TokenIndex &i = parse_context.idx_current_token;

	if (i < token_list.size()) {
		auto current_token = token_list.at(i);
		current_token->index = i;

		if (!parent_node->children.push_back(current_token))
			return gen_oom_syntax_error();

		parse_context.idx_prev_token = i;
		++i;

		return peff::NULLOPT;
	}

	return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), i - 1 }, SyntaxErrorKind::ExpectingMoreTokens);
}

SLKC_API Token *Parser::peek_token(TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	size_t i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = token_list.at(i).get();
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line == TokenIgnoringPolicy::Keep)
					return current_token;
				break;
			case TokenId::Whitespace:
				if (keep_whitespace == TokenIgnoringPolicy::Keep)
					return current_token;
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment == TokenIgnoringPolicy::Keep)
					return current_token;
				break;
			default:
				return current_token;
		}

		++i;
	}

	return token_list.back().get();
}

SLKC_API peff::Option<SyntaxError> Parser::collect_and_expect_token(const GreenNodePin &parent_node, TokenKind token_kind, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), token_kind));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_and_next_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_to_cur_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::split_shr_op_token() {
	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::ShrOp: {
			token->token_id = TokenId::GtOp;

			auto first_angle = global->register_shared_string(token->source_text.get_view().substr(0, 1));
			if (!first_angle)
				return gen_oom_syntax_error();
			token->source_text = first_angle;
			token->source_location.end_position.column -= 1;

			TokenPtr extra_closing_token;
			if (!(extra_closing_token = TokenPtr(peff::alloc_and_construct<Token>(token->allocator.get(), alignof(Token), token->allocator.get(), get_global())))) {
				return gen_oom_syntax_error();
			}

			extra_closing_token->token_id = TokenId::GtOp;
			extra_closing_token->source_location =
				SourceLocation{
					token->source_location.module_node,
					SourcePosition{ token->source_location.begin_position.line, token->source_location.begin_position.column + 1 },
					token->source_location.end_position
				};
			auto second_angle = global->register_shared_string(token->source_text.get_view().substr(1));
			if (!second_angle)
				return gen_oom_syntax_error();
			extra_closing_token->source_text = second_angle;

			if (!token_list.insert(parse_context.idx_current_token + 1, std::move(extra_closing_token))) {
				return gen_oom_syntax_error();
			}

			break;
		}
		default:;
	}

	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::split_rdbrackets_token() {
	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::RDBracket: {
			token->token_id = TokenId::RBracket;

			auto first_bracket = global->register_shared_string(token->source_text.get_view().substr(0, 1));
			if (!first_bracket)
				return gen_oom_syntax_error();
			token->source_text = first_bracket;
			token->source_location.end_position.column -= 1;

			TokenPtr extra_closing_token;
			if (!(extra_closing_token = TokenPtr(peff::alloc_and_construct<Token>(token->allocator.get(), alignof(Token), token->allocator.get(), get_global())))) {
				return gen_oom_syntax_error();
			}

			extra_closing_token->token_id = TokenId::RBracket;
			extra_closing_token->source_location =
				SourceLocation{
					token->source_location.module_node,
					SourcePosition{ token->source_location.begin_position.line, token->source_location.begin_position.column + 1 },
					token->source_location.end_position
				};

			auto second_bracket = global->register_shared_string(token->source_text.get_view().substr(1));
			if (!second_bracket)
				return gen_oom_syntax_error();
			extra_closing_token->source_text = second_bracket;

			if (!token_list.insert(parse_context.idx_current_token + 1, std::move(extra_closing_token))) {
				return gen_oom_syntax_error();
			}

			break;
		}
		default:;
	}

	return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_args(peff::Alloc *allocator, const GreenNodePin &args_node_out, TokenKind terminal_token, TokenKind separator_token) {
	args_node_out->node_kind = GreenNodeKind::Args;

	Token *token;

	if ((token = peek_token())->token_id == terminal_token) {
		co_return peff::NULLOPT;
	}

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, args_node_out, nullptr, 0)(this));

		if ((token = peek_token())->token_id == terminal_token) {
			co_return peff::NULLOPT;
		}

		if ((token = peek_token())->token_id != separator_token) {
			if (!syntax_errors.push_back(
					SyntaxError(
						TokenRange{
							module_node,
							parse_context.idx_current_token },
						ExpectingSingleTokenErrorExData{ separator_token })))
				co_return gen_oom_syntax_error();
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(args_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_subscript_args(peff::Alloc *allocator, const GreenNodePin &args_node_out) {
	args_node_out->node_kind = GreenNodeKind::Args;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(split_rdbrackets_token());

	if ((token = peek_token())->token_id == TokenId::RBracket) {
		co_return peff::NULLOPT;
	}

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, args_node_out, nullptr, 0)(this));

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, args_node_out, nullptr, 0)(this));

		SLKC_CO_RETURN_IF_PARSE_ERROR(split_rdbrackets_token());

		if ((token = peek_token())->token_id == TokenId::RBracket) {
			co_return peff::NULLOPT;
		}

		if ((token = peek_token())->token_id != TokenId::Comma) {
			if (!syntax_errors.push_back(
					SyntaxError(
						TokenRange{
							module_node,
							parse_context.idx_current_token },
						ExpectingSingleTokenErrorExData{ TokenId::Comma })))
				co_return gen_oom_syntax_error();
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(args_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_type_name(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin type_name_node_out;
	if (!(type_name_node_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	type_name_node_out->node_kind = GreenNodeKind::TypeName;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &type_name_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = type_name_node_out;
	});

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, type_name_node_out);

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::I8TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::I8TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I16TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::I16TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I32TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::I32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I64TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::I64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::ISizeTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::ISizeTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U8TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::U8TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U16TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::U16TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U32TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::U32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U64TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::U64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::USizeTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::USizeTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::F32TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::F32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::F64TypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::F64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::StringTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::StringTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::BoolTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::BoolTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::VoidTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::VoidTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::ObjectTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::ObjectTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::AnyTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::AnyTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::NeverTypeName:
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::NeverTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::Id: {
			type_name_node_out->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::CustomTypeName };

			GreenNodePin id_ref;

			if (!(id_ref = make_green_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(type_name_node_out, id_ref);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(allocator, id_ref, false)(this));
			break;
		}
		default:
			co_return SyntaxError{ TokenRange{ module_node, token->index }, SyntaxErrorKind::UnexpectedToken };
	}

	if ((token = peek_token())->token_id == TokenId::ObjectTypeName) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
	}

	while (true) {
		switch ((token = peek_token())->token_id) {
			case TokenId::ConstKeyword:
			case TokenId::MutableKeyword:
				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
				break;
			default:
				break;
		}

		if ((token = peek_token())->token_id == TokenId::FinalKeyword) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
		}

		if ((token = peek_token())->token_id == TokenId::LocalKeyword) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
		}

		switch ((token = peek_token())->token_id) {
			case TokenId::RestrictKeyword:
			case TokenId::MultiKeyword:
			case TokenId::SynchronizedKeyword:
				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
				break;
			default:
				break;
		}

		if ((token = peek_token())->token_id == TokenId::Question) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
		}

		if ((token = peek_token())->token_id == TokenId::LBracket) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));

			auto new_tn = make_green_node(get_global());

			if (!new_tn)
				co_return gen_oom_syntax_error();

			new_tn->node_kind = GreenNodeKind::TypeName;
			new_tn->exdata = TypeNameGreenNodeExData{ GreenNodeTypeNameKind::ArrayTypeName };

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(new_tn, type_name_node_out);

			type_name_node_out = new_tn;

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(type_name_node_out, TokenId::RBracket));
		}

		break;
	}

	if ((token = peek_token())->token_id == TokenId::RefKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
	}

	if ((token = peek_token())->token_id == TokenId::ReadonlyKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
	}

	if ((token = peek_token())->token_id == TokenId::LocalKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_id_ref_entry(peff::Alloc *allocator, const GreenNodePin &id_ref_entry_node_out, bool requires_generic_distinguisher) {
	id_ref_entry_node_out->node_kind = GreenNodeKind::IdRefEntry;

	Token *token;
	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::Id));

	token = peek_token();
	if (token->token_id == TokenId::ScopeOp) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::LtOp));

		while (true) {
			GreenNodePin tn;

			if (!(tn = make_green_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(id_ref_entry_node_out, tn);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, tn, nullptr)(this));
			if ((token = peek_token())->token_id != TokenId::Comma)
				break;
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		}
		SLKC_CO_RETURN_IF_PARSE_ERROR(split_shr_op_token());
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::GtOp));
	} else if ((requires_generic_distinguisher) && (token->token_id == TokenId::LtOp)) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		while (true) {
			GreenNodePin tn;

			if (!(tn = make_green_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(id_ref_entry_node_out, tn);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, tn, nullptr)(this));
			if ((token = peek_token())->token_id != TokenId::Comma)
				break;
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		}
		SLKC_CO_RETURN_IF_PARSE_ERROR(split_shr_op_token());
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::GtOp));
	}
	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_id_ref(peff::Alloc *allocator, const GreenNodePin &id_ref_node_out, bool requires_generic_distinguisher) {
	id_ref_node_out->node_kind = GreenNodeKind::IdRef;

	Token *token;

	switch ((token = peek_token())->token_id) {
		case TokenId::ThisKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
			if ((token = peek_token())->token_id != TokenId::Dot)
				co_return peff::NULLOPT;
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
			break;
		case TokenId::ScopeOp:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
			break;
		default:
			break;
	}

	while (true) {
		GreenNodePin node_pin;

		if (!(node_pin = make_green_node(get_global())))
			co_return gen_oom_syntax_error();

		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(id_ref_node_out, node_pin);

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref_entry(allocator, node_pin, requires_generic_distinguisher)(this));

		if ((token = peek_token())->token_id != TokenId::Dot)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_inheritance_slot(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin slot_out;
	if (!(slot_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = GreenNodeKind::InheritanceSlot;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::LParenthesis) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, slot_out, nullptr)(this));

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(slot_out, TokenId::RParenthesis));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_impl_item(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin slot_out;
	if (!(slot_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = GreenNodeKind::ImplItem;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::TraitKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));
	}

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, slot_out, nullptr)(this));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_impl_list(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin slot_out;
	if (!(slot_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = GreenNodeKind::ImplItem;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::Colon) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));

		while (true) {
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_item(allocator, slot_out, nullptr)(this));

			if ((token = peek_token())->token_id != TokenId::AddOp)
				break;

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));
		}
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_operator_name(peff::Alloc *allocator, const GreenNodePin &parent_node) {
	GreenNodePin name_node_out;
	if (!(name_node_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	name_node_out->node_kind = GreenNodeKind::TypeName;

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent_node, name_node_out);

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::AddOp:
		case TokenId::SubOp:
		case TokenId::MulOp:
		case TokenId::DivOp:
		case TokenId::ModOp:
		case TokenId::AndOp:
		case TokenId::OrOp:
		case TokenId::XorOp:
		case TokenId::LAndOp:
		case TokenId::LOrOp:
		case TokenId::LtOp:
		case TokenId::GtOp:
		case TokenId::LtEqOp:
		case TokenId::GtEqOp:
		case TokenId::CmpOp:
		case TokenId::EqOp:
		case TokenId::NeqOp:
		case TokenId::ShlOp:
		case TokenId::ShrOp:
		case TokenId::AddAssignOp:
		case TokenId::SubAssignOp:
		case TokenId::MulAssignOp:
		case TokenId::DivAssignOp:
		case TokenId::ModAssignOp:
		case TokenId::AndAssignOp:
		case TokenId::OrAssignOp:
		case TokenId::XorAssignOp:
		case TokenId::ShlAssignOp:
		case TokenId::ShrAssignOp:
		case TokenId::NotOp:
		case TokenId::LNotOp:
		case TokenId::NewKeyword:
		case TokenId::DeleteKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(name_node_out));
			break;
		case TokenId::LParenthesis:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(name_node_out));
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(name_node_out, TokenId::RParenthesis));
			break;
		case TokenId::LBracket:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(name_node_out));
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(name_node_out, TokenId::RBracket));
			break;
		default:
			if (!syntax_errors.push_back(
					SyntaxError(
						TokenRange{
							module_node,
							parse_context.idx_current_token },
						SyntaxErrorKind::UnexpectedToken)))
				co_return gen_oom_syntax_error();
	}
	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_fn(peff::Alloc *allocator, const GreenNodePin &fn_node) {
	fn_node->node_kind = GreenNodeKind::FnDef;

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::Id));
			break;
		case TokenId::OperatorKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_operator_name(allocator, fn_node)(this));
			break;
		default:
			std::terminate();
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::LParenthesis));

	if ((token = peek_token())->token_id != TokenId::RParenthesis)
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(allocator, fn_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::RParenthesis));

	if ((token = peek_token())->token_id == TokenId::ConstKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
	}

	if ((token = peek_token())->token_id == TokenId::RestrictKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
	}

	if ((token = peek_token())->token_id == TokenId::VirtualKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
	}

	if ((token = peek_token())->token_id == TokenId::OverrideKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
		if ((token = peek_token())->token_id == TokenId::LParenthesis) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, fn_node, nullptr)(this));
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::RParenthesis));
		}
	}

	if ((token = peek_token())->token_id == TokenId::ReturnTypeOp) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(allocator, fn_node, nullptr)(this));
	}

	switch ((token = peek_token())->token_id) {
		case TokenId::LBrace:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));

			while (true) {
				if ((token = peek_token())->token_id == TokenId::RBrace)
					break;

				if (auto e = co_await parse_stmt(allocator, fn_node, nullptr)(this); e.has_value()) {
					if (!syntax_errors.push_back(std::move(e).value()))
						co_return gen_oom_syntax_error();
				}
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::RBrace));
			break;
		case TokenId::Semicolon:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
			fn_node->node_kind = GreenNodeKind::FnDecl;
			break;
		default:
			if (!syntax_errors.push_back(
					SyntaxError(
						TokenRange{
							module_node,
							parse_context.idx_current_token },
						SyntaxErrorKind::UnexpectedToken)))
				co_return gen_oom_syntax_error();
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_class(peff::Alloc *allocator, const GreenNodePin &cls_node) {
	cls_node->node_kind = GreenNodeKind::ClassDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::ClassKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(allocator, cls_node, nullptr)(this));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(allocator, cls_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::LBrace));

	while (true) {
		if (auto syntax_error = co_await parse_program_stmt(allocator, cls_node)(this); syntax_error.has_value()) {
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_interface(peff::Alloc *allocator, const GreenNodePin &interface_node) {
	interface_node->node_kind = GreenNodeKind::InterfaceDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::InterfaceKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(allocator, interface_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::LBrace));

	while (true) {
		if (auto syntax_error = co_await parse_program_stmt(allocator, interface_node)(this); syntax_error.has_value()) {
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_trait(peff::Alloc *allocator, const GreenNodePin &trait_node) {
	trait_node->node_kind = GreenNodeKind::TraitDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::TraitKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(allocator, trait_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::LBrace));

	while (true) {
		if (auto syntax_error = co_await parse_program_stmt(allocator, trait_node)(this); syntax_error.has_value()) {
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_except(peff::Alloc *allocator, const GreenNodePin &except_node) {
	except_node->node_kind = GreenNodeKind::ClassDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(except_node, TokenId::ExceptKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(except_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(allocator, except_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(except_node, TokenId::LBrace));

	while (true) {
		if (auto syntax_error = co_await parse_program_stmt(allocator, except_node)(this); syntax_error.has_value()) {
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(except_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_struct(peff::Alloc *allocator, const GreenNodePin &struct_node) {
	struct_node->node_kind = GreenNodeKind::StructDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(struct_node, TokenId::StructKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(struct_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(allocator, struct_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(struct_node, TokenId::LBrace));

	while (true) {
		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;

		if (auto syntax_error = co_await parse_program_stmt(allocator, struct_node)(this); syntax_error.has_value()) {
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(struct_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_const_enum(peff::Alloc *allocator, const GreenNodePin &enum_node) {
	enum_node->node_kind = GreenNodeKind::ConstEnumDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::ConstKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(allocator, enum_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::LBrace));

	while (true) {
		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_const_and_scoped_enum_item(allocator, enum_node, nullptr)(this));

		if ((token = peek_token())->token_id != TokenId::Comma)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(enum_node));
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_scoped_enum(peff::Alloc *allocator, const GreenNodePin &enum_node) {
	enum_node->node_kind = GreenNodeKind::ScopedEnumDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(allocator, enum_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::LBrace));

	while (true) {
		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_const_and_scoped_enum_item(allocator, enum_node, nullptr)(this));

		if ((token = peek_token())->token_id != TokenId::Comma)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(enum_node));
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_const_and_scoped_enum_item(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin item_node_out;
	if (!(item_node_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	item_node_out->node_kind = GreenNodeKind::ConstAndScopedEnumItem;

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, item_node_out);

	peff::Deferred put_node_pin_out_guard([node_pin_out, &item_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = item_node_out;
	});

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(item_node_out, TokenId::Id));

	Token *token;
	if ((token = peek_token())->token_id == TokenId::AssignOp) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(item_node_out));

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(allocator, item_node_out, nullptr, 0)(this));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_union_enum(peff::Alloc *allocator, const GreenNodePin &enum_node) {
	enum_node->node_kind = GreenNodeKind::ScopedEnumDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::UnionKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(allocator, enum_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::LBrace));

	while (true) {
		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_union_enum_case(allocator, enum_node, nullptr)(this));

		if ((token = peek_token())->token_id != TokenId::Comma)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(enum_node));
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(enum_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_union_enum_case(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out) {
	GreenNodePin case_node_out;
	if (!(case_node_out = make_green_node(get_global())))
		co_return gen_oom_syntax_error();
	case_node_out->node_kind = GreenNodeKind::UnionEnumCase;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &case_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = case_node_out;
	});

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, case_node_out);

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node_out, TokenId::CaseKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node_out, TokenId::Id));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node_out, TokenId::LParenthesis));
	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(allocator, case_node_out, nullptr)(this));
	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(case_node_out, TokenId::RParenthesis));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_program_stmt(peff::Alloc *allocator, const GreenNodePin &module_node) {
	Token *token;

	GreenNodePin member;
	if (!(member = make_green_node(get_global())))
		co_return gen_oom_syntax_error();

	SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(module_node, member);

	switch ((token = peek_token())->token_id) {
		case TokenId::PublicKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
			break;
		case TokenId::PrivateKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
			break;
		case TokenId::ProtectedKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
			break;
		default:
			break;
	}

	if ((token = peek_token())->token_id == TokenId::NativeKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
	}

	if ((token = peek_token())->token_id == TokenId::StaticKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
	}

	switch ((token = peek_token())->token_id) {
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
		case TokenId::OperatorKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_fn(allocator, member)(this));
			break;
		case TokenId::ClassKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_class(allocator, member)(this));
			break;
		case TokenId::InterfaceKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_interface(allocator, member)(this));
			break;
		case TokenId::TraitKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_trait(allocator, member)(this));
			break;
		case TokenId::ExceptKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_except(allocator, member)(this));
			break;
		case TokenId::StructKeyword:
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_struct(allocator, member)(this));
			break;
		case TokenId::VarKeyword:
		case TokenId::LetKeyword: {
			member->node_kind = GreenNodeKind::GlobalVar;

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(allocator, member, nullptr)(this));

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(member, TokenId::Semicolon));
			break;
		}
		case TokenId::EnumKeyword: {
			member->node_kind = GreenNodeKind::UnknownEnumDecl;

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));

			switch ((token = peek_token())->token_id) {
				case TokenId::ConstKeyword: {
					SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_const_enum(allocator, member)(this));
					break;
				}
				case TokenId::Id: {
					SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_scoped_enum(allocator, member)(this));
					break;
				}
				case TokenId::UnionKeyword: {
					SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_union_enum(allocator, member)(this));
					break;
				}
				default:
					co_return SyntaxError{ TokenRange{ module_node, token->index }, SyntaxErrorKind::UnexpectedToken };
			}
			break;
		}
		case TokenId::ImportKeyword: {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));

			{
				GreenNodePin module_name;

				if (!(module_name = make_green_node(get_global())))
					co_return gen_oom_syntax_error();
				SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(member, module_name);
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(allocator, module_name, false)(this));
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(member, TokenId::Semicolon));
			break;
		}
		default:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));
			co_return SyntaxError{ TokenRange{ module_node, token->index }, SyntaxErrorKind::UnexpectedToken };
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_program(peff::Alloc *allocator, const GreenNodePin &module_node) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(module_node));

		{
			GreenNodePin module_name;

			if (!(module_name = make_green_node(get_global())))
				co_return gen_oom_syntax_error();
			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(module_node, module_name);
			if ((syntax_error = (co_await parse_id_ref(allocator, module_name, false)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_oom_syntax_error();
				syntax_error.reset();
			}
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(module_name, TokenId::Semicolon));
		}
	}

	while ((t = peek_token())->token_id != TokenId::End) {
		if ((syntax_error = (co_await parse_program_stmt(allocator, module_node)(this))).has_value()) {
			//co_return std::move(syntax_error).value();
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntax_errors.push_back(std::move(syntax_error).value()))
				co_return gen_oom_syntax_error();
		}
	}

	// Collect trailing tokens.
	SLKC_CO_RETURN_IF_PARSE_ERROR(to_next_token(module_node));

	co_return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::parse(const GreenNodePin &root_node) {
	return parse_program(get_global()->get_allocator(), root_node).resume(this);
}
