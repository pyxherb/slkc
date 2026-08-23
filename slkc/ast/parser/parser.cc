#include "parser.h"

using namespace slkc;
using namespace slkc::ast;

SLAKE_FORCEINLINE peff::Option<SyntaxError> ParseCoroutine::resume(Parser *parser) {
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

SLKC_API peff::Option<SyntaxError> Parser::to_next_token(const RGNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	TokenIndex &i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		auto current_token = token_list.at(i);
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return peff::NULLOPT;
				}
				break;
			case TokenId::Whitespace:
				if (keep_whitespace == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return peff::NULLOPT;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment == TokenIgnoringPolicy::Keep) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return peff::NULLOPT;
				}
				break;
			default:
				parse_context.idx_prev_token = parse_context.idx_current_token;
				++i;
				return peff::NULLOPT;
		}

		RGNodePin new_terminal_node = make_rg_node(get_global());
		if (!new_terminal_node)
			return gen_oom_syntax_error();

		new_terminal_node->source_token = current_token;

		if (!parent_node->push_child(new_terminal_node))
			return gen_oom_syntax_error();

		++i;
	}

	return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), i - 1 }, SyntaxErrorKind::ExpectingMoreTokens);
}

SLKC_API void Parser::next_token() {
	TokenIndex &i = parse_context.idx_current_token;

	if (i < token_list.size()) {
		auto current_token = token_list.at(i);
		current_token->index = i;

		parse_context.idx_prev_token = i;
		++i;
	}
}

SLKC_API peff::Option<SyntaxError> Parser::collect_token(const RGNodePin &parent_node) {
	TokenIndex &i = parse_context.idx_current_token;

	if (i < token_list.size()) {
		auto current_token = token_list.at(i);
		current_token->index = i;

		RGNodePin new_terminal_node = make_rg_node(get_global());
		if (!new_terminal_node)
			return gen_oom_syntax_error();

		new_terminal_node->source_token = current_token;

		if (!parent_node->push_child(new_terminal_node))
			return gen_oom_syntax_error();

		parse_context.idx_prev_token = i;
		++i;
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

SLKC_API peff::Option<SyntaxError> Parser::collect_and_expect_token(const RGNodePin &parent_node, TokenKind token_kind, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), token_kind));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_and_next_token(const RGNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_to_cur_token(const RGNodePin &parent_node, TokenIgnoringPolicy keep_new_line, TokenIgnoringPolicy keep_whitespace, TokenIgnoringPolicy keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::split_shr_op_token() {
	switch (Token *token = peek_token(); token->token_id) {
		case TokenId::ShrOp: {
			token->token_id = TokenId::GtOp;

			auto first_angle = global->register_shared_string(token->source_text.get().substr(0, 1));
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
			auto second_angle = global->register_shared_string(token->source_text.get().substr(1));
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

			auto first_bracket = global->register_shared_string(token->source_text.get().substr(0, 1));
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

			auto second_bracket = global->register_shared_string(token->source_text.get().substr(1));
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

SLKC_API ParseCoroutine Parser::parse_args(const RGNodePin &args_node_out, TokenKind terminal_token, TokenKind separator_token) {
	args_node_out->node_kind = RGNodeKind::Args;

	Token *token;

	if ((token = peek_token())->token_id == terminal_token) {
		co_return peff::NULLOPT;
	}

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(args_node_out, nullptr, 0)(this));

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

SLKC_API ParseCoroutine Parser::parse_subscript_args(const RGNodePin &args_node_out) {
	args_node_out->node_kind = RGNodeKind::Args;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(split_rdbrackets_token());

	if ((token = peek_token())->token_id == TokenId::RBracket) {
		co_return peff::NULLOPT;
	}

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(args_node_out, nullptr, 0)(this));

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_expr(args_node_out, nullptr, 0)(this));

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

SLKC_API ParseCoroutine Parser::parse_type_name(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin type_name_node_out;
	if (!(type_name_node_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	type_name_node_out->node_kind = RGNodeKind::TypeName;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &type_name_node_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = type_name_node_out;
	});

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, type_name_node_out);

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::I8TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::I8TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I16TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::I16TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I32TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::I32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::I64TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::I64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::ISizeTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::ISizeTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U8TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::U8TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U16TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::U16TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U32TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::U32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::U64TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::U64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::USizeTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::USizeTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::F32TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::F32TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::F64TypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::F64TypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::StringTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::StringTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::BoolTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::BoolTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::VoidTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::VoidTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::ObjectTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::ObjectTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::AnyTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::AnyTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::NeverTypeName:
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::NeverTypeName };
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(type_name_node_out));
			break;
		case TokenId::Id: {
			type_name_node_out->exdata = TypeNameRGNodeExData{ RGTypeNameKind::CustomTypeName };

			RGNodePin id_ref;

			if (!(id_ref = make_rg_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(type_name_node_out, id_ref);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref(id_ref, false)(this));
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

			auto new_tn = make_rg_node(get_global());

			if (!new_tn)
				co_return gen_oom_syntax_error();

			new_tn->node_kind = RGNodeKind::TypeName;
			new_tn->exdata = TypeNameRGNodeExData{ RGTypeNameKind::ArrayTypeName };

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

SLKC_API ParseCoroutine Parser::parse_id_ref_entry(const RGNodePin &id_ref_entry_node_out, bool requires_generic_distinguisher) {
	id_ref_entry_node_out->node_kind = RGNodeKind::IdRefEntry;

	Token *token;
	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::Id));

	token = peek_token();
	if (((requires_generic_distinguisher) && (token->token_id == TokenId::LtOp)) || (token->token_id == TokenId::ScopeOp)) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		if (token->token_id == TokenId::ScopeOp) {
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::LtOp));
		}
		while (true) {
			RGNodePin tn;

			if (!(tn = make_rg_node(get_global())))
				co_return gen_oom_syntax_error();

			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(id_ref_entry_node_out, tn);

			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(tn, nullptr)(this));
			if ((token = peek_token())->token_id != TokenId::Comma)
				break;
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_entry_node_out));
		}
		SLKC_CO_RETURN_IF_PARSE_ERROR(split_shr_op_token());
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::GtOp));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_id_ref(const RGNodePin &id_ref_node_out, bool requires_generic_distinguisher) {
	id_ref_node_out->node_kind = RGNodeKind::IdRef;

	Token *token;

	switch ((token = peek_token())->token_id) {
		case TokenId::ThisKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
			break;
		case TokenId::ScopeOp:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
			break;
		default:
			break;
	}

	while (true) {
		RGNodePin node_pin;

		if (!(node_pin = make_rg_node(get_global())))
			co_return gen_oom_syntax_error();

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_id_ref_entry(node_pin, requires_generic_distinguisher)(this));

		if ((token = peek_token())->token_id != TokenId::Dot)
			break;

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(id_ref_node_out));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_inheritance_slot(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin slot_out;
	if (!(slot_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = RGNodeKind::InheritanceSlot;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::LParenthesis) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));

		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(slot_out, nullptr)(this));

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(slot_out, TokenId::RParenthesis));
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_impl_item(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin slot_out;
	if (!(slot_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = RGNodeKind::ImplItem;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::TraitKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));
	}

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(slot_out, nullptr)(this));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_impl_list(RGNodePin parent, RGNodePin *node_pin_out) {
	RGNodePin slot_out;
	if (!(slot_out = make_rg_node(get_global())))
		co_return gen_oom_syntax_error();
	slot_out->node_kind = RGNodeKind::ImplItem;

	peff::Deferred put_node_pin_out_guard([node_pin_out, &slot_out]() noexcept {
		if (node_pin_out)
			*node_pin_out = slot_out;
	});

	if (parent)
		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(parent, slot_out);

	Token *token;
	if ((token = peek_token())->token_id == TokenId::Colon) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));

		while (true) {
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_item(slot_out, nullptr)(this));

			if ((token = peek_token())->token_id != TokenId::AddOp)
				break;

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(slot_out));
		}
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_fn(const RGNodePin &fn_node) {
	fn_node->node_kind = RGNodeKind::FnDef;

	Token *token;
	switch ((token = peek_token())->token_id) {
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
			break;
		default:
			std::terminate();
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::Id));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::LParenthesis));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding(fn_node, nullptr)(this));

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
			SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(fn_node, nullptr)(this));
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::RParenthesis));
		}
	}

	if ((token = peek_token())->token_id == TokenId::ReturnTypeOp) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_type_name(fn_node, nullptr)(this));
	}

	switch ((token = peek_token())->token_id) {
		case TokenId::LBrace:
			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(fn_node));

			while (true) {
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_stmt(fn_node, nullptr)(this));

				if ((token = peek_token())->token_id == TokenId::RBrace)
					break;
			}

			SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(fn_node, TokenId::RBrace));
			break;
		case TokenId::Semicolon:
			fn_node->node_kind = RGNodeKind::FnDecl;
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

SLKC_API ParseCoroutine Parser::parse_class(const RGNodePin &cls_node) {
	cls_node->node_kind = RGNodeKind::ClassDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::ClassKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_inheritance_slot(cls_node, nullptr)(this));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(cls_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::LBrace));

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_program_stmt(cls_node)(this));

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(cls_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_interface(const RGNodePin &interface_node) {
	interface_node->node_kind = RGNodeKind::InterfaceDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::InterfaceKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(interface_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::LBrace));

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_program_stmt(interface_node)(this));

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(interface_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_trait(const RGNodePin &trait_node) {
	trait_node->node_kind = RGNodeKind::TraitDef;

	Token *token;

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::InterfaceKeyword));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::Id));

	SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_impl_list(trait_node, nullptr)(this));

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::LBrace));

	while (true) {
		SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_program_stmt(trait_node)(this));

		if ((token = peek_token())->token_id == TokenId::RBrace)
			break;
	}

	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(trait_node, TokenId::RBrace));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_program_stmt(const RGNodePin &module_node) {
	Token *token;

	while (true) {
		if ((token = peek_token())->token_id == TokenId::End)
			break;

		RGNodePin member;
		if (!(member = make_rg_node(get_global())))
			co_return gen_oom_syntax_error();

		SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(module_node, member);

		switch (token->token_id) {
			case TokenId::FnKeyword:
			case TokenId::AsyncKeyword:
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_fn(member)(this));
				break;
			case TokenId::ClassKeyword:
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_class(member)(this));
				break;
			case TokenId::InterfaceKeyword:
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_interface(member)(this));
				break;
			case TokenId::TraitKeyword:
				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_trait(member)(this));
				break;
			case TokenId::VarKeyword:
			case TokenId::LetKeyword: {
				member->node_kind = RGNodeKind::GlobalVar;

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(member));

				SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(parse_var_binding_list(member, nullptr)(this));

				SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(member, TokenId::Semicolon));
				break;
			}
			default:
				co_return SyntaxError{ TokenRange{ module_node, token->index }, SyntaxErrorKind::UnexpectedToken };
		}
	}
}

SLKC_API ParseCoroutine Parser::parse_program(const RGNodePin &module_node) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(module_node));

		{
			RGNodePin module_name;
			if ((syntax_error = (co_await parse_id_ref(module_name, false)(this)))) {
				if (!syntax_errors.push_back(std::move(syntax_error.value())))
					co_return gen_oom_syntax_error();
				syntax_error.reset();
			}
			SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(module_node, module_name);
		}

		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(module_node, TokenId::Semicolon));
	}

	while ((t = peek_token())->token_id != TokenId::End) {
		if ((syntax_error = (co_await parse_program_stmt(module_node)(this)))) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
		}
	}

	// Collect trailing tokens.
	SLKC_CO_RETURN_IF_PARSE_ERROR(to_next_token(module_node));

	co_return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::parse(const RGNodePin &root_node) {
	return parse_program(root_node).resume(this);
}
