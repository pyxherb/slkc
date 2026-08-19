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

SLKC_API peff::Option<SyntaxError> Parser::to_next_token(const RGNodePin &parent_node, bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	TokenIndex &i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		auto current_token = token_list.at(i);
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return peff::NULLOPT;
				}
				break;
			case TokenId::Whitespace:
				if (keep_whitespace) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return peff::NULLOPT;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment) {
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

	return peff::NULLOPT;
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
}

SLKC_API Token *Parser::peek_token(bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	size_t i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = token_list.at(i).get();
		current_token->index = i;

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line)
					return current_token;
				break;
			case TokenId::Whitespace:
				if (keep_whitespace)
					return current_token;
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment)
					return current_token;
				break;
			default:
				return current_token;
		}

		++i;
	}

	return token_list.back().get();
}

SLKC_API peff::Option<SyntaxError> Parser::collect_and_expect_token(const RGNodePin &parent_node, TokenKind token_kind, bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(expect_token(peek_token(), token_kind));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_and_next_token(const RGNodePin &parent_node, bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	SLKC_RETURN_IF_PARSE_ERROR(to_next_token(parent_node, keep_new_line, keep_whitespace, keep_comment));
	SLKC_RETURN_IF_PARSE_ERROR(collect_token(parent_node));
	return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::collect_to_cur_token(const RGNodePin &parent_node, bool keep_new_line, bool keep_whitespace, bool keep_comment) {
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

SLKC_API ParseCoroutine Parser::parse_id_ref_entry(RGNodePin &id_ref_entry_node_out) {
	id_ref_entry_node_out = make_rg_node(get_global());

	if (!id_ref_entry_node_out)
		co_return gen_oom_syntax_error();

	Token *token;
	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_entry_node_out, TokenId::Id));

	// TODO: Implement it.

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_id_ref(RGNodePin &id_ref_node_out) {
	id_ref_node_out = make_rg_node(get_global());

	if (!id_ref_node_out)
		co_return gen_oom_syntax_error();

	Token *token;
	SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_expect_token(id_ref_node_out, TokenId::Id));

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_program_stmt(const RGNodePin &module_node) {
}

SLKC_API ParseCoroutine Parser::parse_program(const RGNodePin &module_node) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		SLKC_CO_RETURN_IF_PARSE_ERROR(collect_and_next_token(module_node));

		{
			RGNodePin module_name;
			if ((syntax_error = (co_await parse_id_ref(module_name)(this)))) {
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
