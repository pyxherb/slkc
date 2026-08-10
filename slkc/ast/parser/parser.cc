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

SLKC_API ParseCoroutine Parser::parse_program_stmt(peff::Alloc *allocator) {
	peff::Option<SyntaxError> syntax_error;

	peff::DynArray<NodePtr<AttributeNode>> attributes(allocator);

	SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_attributes(allocator, attributes));

	AccessModifier access;
	Token *current_token;

	for (;;) {
		switch ((current_token = peek_token())->token_id) {
			case TokenId::PublicKeyword:
				access.visibility = Visibility::Public;
				next_token();
				break;
			case TokenId::PrivateKeyword:
				access.visibility = Visibility::Private;
				next_token();
				break;
			case TokenId::ProtectedKeyword:
				access.visibility = Visibility::Protected;
				next_token();
				break;
			case TokenId::StaticKeyword:
				access.is_static = true;
				next_token();
				break;
			case TokenId::NativeKeyword:
				access.is_native = true;
				next_token();
				break;
			default:
				goto access_modifier_parse_end;
		}
	}

access_modifier_parse_end:
	Token *token = peek_token();

	NodePin<ModuleNode> p = cur_parent.cast_to<ModuleNode>();

	if (p->get_ast_node_type() == NodeType::Module) {
		access.is_static = true;
	}

	auto p_scope = p->get_scope();

	switch (token->token_id) {
		case TokenId::EnumKeyword: {
			next_token();
			switch (peek_token()->token_id) {
				case TokenId::UnionKeyword: {
					next_token();
					NodePin<UnionEnumNode> enum_node;

					if (!(enum_node = make_node<UnionEnumNode>(get_global())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					peff::Deferred set_token_range_guard([this, token, enum_node]() noexcept {
						enum_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
					});

					Token *name_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

					next_token();

					size_t idx_member;
					if ((idx_member = p_scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
						co_return gen_oom_syntax_error();
					}

					if (!enum_node->set_name(name_token->source_text)) {
						co_return gen_oom_syntax_error();
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_union_enum_item(allocator, enum_node.cast_to<MemberNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p_scope->members_index.find(enum_node->get_name()); it != p_scope->members_index.end()) {
						peff::String s(allocator);

						if (!s.build(enum_node->get_name())) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->get_token_range(), std::move(ex_data));
					} else {
						if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
							co_return scope_member_op_result_to_syntax_error(result);
						}
					}
					break;
				}
				case TokenId::ConstKeyword: {
					next_token();
					NodePin<ConstEnumNode> enum_node;

					if (!(enum_node = make_node<ConstEnumNode>(get_global())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					peff::Deferred set_token_range_guard([this, token, enum_node]() noexcept {
						enum_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
					});

					Token *name_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

					next_token();

					size_t idx_member;
					if ((idx_member = p_scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
						co_return gen_oom_syntax_error();
					}

					if (!enum_node->set_name(name_token->source_text)) {
						co_return gen_oom_syntax_error();
					}

					if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
						next_token();

						TypeName underlying_type;
						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, underlying_type));

						enum_node->get_scope()->underlying_type = underlying_type;

						Token *r_parenthese_token;
						SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

						next_token();
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_enum_item(allocator, enum_node.cast_to<MemberNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p_scope->members_index.find(enum_node->get_name()); it != p_scope->members_index.end()) {
						peff::String s(allocator);

						if (!s.build(enum_node->get_name())) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->get_token_range(), std::move(ex_data));
					} else {
						if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
							co_return scope_member_op_result_to_syntax_error(result);
						}
					}
					break;
				}
				case TokenId::Id: {
					Token *name_token = next_token();
					NodePin<ScopedEnumNode> enum_node;

					if (!(enum_node = make_node<ScopedEnumNode>(get_global())))
						co_return gen_oom_syntax_error();

					if (!(enum_node->alloc_scope()))
						co_return gen_oom_syntax_error();

					size_t idx_member;
					{
						peff::Deferred set_token_range_guard([this, token, enum_node]() noexcept {
							enum_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
						});
						if ((idx_member = p_scope->push_member(enum_node.cast_to<MemberNode>())) == SIZE_MAX) {
							co_return gen_oom_syntax_error();
						}

						if (!enum_node->set_name(name_token->source_text)) {
							co_return gen_oom_syntax_error();
						}

						if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
							next_token();

							TypeName underlying_type;
							SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, underlying_type));

							enum_node->get_scope()->underlying_type = underlying_type;

							Token *r_parenthese_token;
							SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

							next_token();
						}
					}

					Token *l_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

					next_token();

					while (true) {
						if (peek_token()->token_id == TokenId::RBrace)
							break;

						if ((syntax_error = (co_await parse_enum_item(allocator, enum_node.cast_to<MemberNode>())(this)))) {
							if (syntax_error->error_kind == SyntaxErrorKind::OutOfMemory)
								co_return syntax_error;
							if (!syntax_errors.push_back(syntax_error.move()))
								co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::Comma)
							break;
						Token *comma_token = next_token();
					}

					Token *r_brace_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

					next_token();

					if (auto it = p_scope->members_index.find(enum_node->get_name()); it != p_scope->members_index.end()) {
						peff::String s(allocator);

						if (!s.build(enum_node->get_name())) {
							co_return gen_oom_syntax_error();
						}

						ConflictingDefinitionsErrorExData ex_data(std::move(s));

						co_return SyntaxError(enum_node->get_token_range(), std::move(ex_data));
					} else {
						if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
							co_return scope_member_op_result_to_syntax_error(result);
						}
					}
					break;
				}
				default:
					co_return SyntaxError(TokenRange{ parse_context.mod, token->index }, SyntaxErrorKind::UnexpectedToken);
			}
			break;
		}
		case TokenId::AttributeKeyword: {
			// Attribute definition.
			next_token();

			NodePin<AttributeNode> attribute_node;

			if (!(attribute_node = make_node<AttributeNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(attribute_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			attribute_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			size_t idx_member;
			if ((idx_member = p_scope->push_member(attribute_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			if (!attribute_node->set_name(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::Deferred set_token_range_guard([this, token, attribute_node]() noexcept {
					attribute_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
				});

				NodePin<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = attribute_node.cast_to<MemberNode>();

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(allocator)(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p_scope->members_index.find(attribute_node->get_name()); it != p_scope->members_index.end()) {
				peff::String s(allocator);

				if (!s.build(attribute_node->get_name())) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(attribute_node->get_token_range(), std::move(ex_data));
			} else {
				if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}
			}

			break;
		}
		case TokenId::FnKeyword:
		case TokenId::AsyncKeyword:
		case TokenId::OperatorKeyword:
		case TokenId::DefKeyword: {
			// Function.
			NodePin<FnOverloadingNode> fn;

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_fn(allocator, fn));

			fn->access_modifier = access;

			if (auto it = p_scope->members_index.find(fn->get_name()); it != p_scope->members_index.end()) {
				auto pinned_m = p_scope->members.at(it.value()).pin();
				if (!pinned_m) {
					switch (pinned_m.get_fail_reason()) {
						case PinFailReason::OutOfMemory:
							co_return gen_oom_syntax_error();
							break;
						case PinFailReason::IOError:
							co_return gen_pinning_io_error();
							break;
						default:
							co_return SyntaxError(TokenRange{ parse_context.mod, token->index }, SyntaxErrorKind::UnexpectedToken);
							break;
					}
				}
				if (pinned_m->get_ast_node_type() != NodeType::Fn) {
					peff::String s(allocator);

					if (!s.build(fn->get_name())) {
						co_return gen_oom_syntax_error();
					}

					ConflictingDefinitionsErrorExData ex_data(std::move(s));

					co_return SyntaxError(fn->get_token_range(), std::move(ex_data));
				}
				auto fn_slot = pinned_m.cast_to<FnNode>();
				fn_slot->set_parent(fn_slot);
				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					co_return gen_oom_syntax_error();
				}
			} else {
				NodePin<FnNode> fn_slot;

				if (!(fn_slot = make_node<FnNode>(get_global()))) {
					co_return gen_oom_syntax_error();
				}

				fn_slot->set_name(fn->get_name());

				if (auto result = p_scope->add_member(fn_slot.cast_to<MemberNode>()); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}

				fn->set_parent(fn_slot.get_index());

				if (!fn_slot->overloadings.push_back(std::move(fn))) {
					co_return gen_oom_syntax_error();
				}
			}
			break;
		}
		case TokenId::ClassKeyword: {
			// Class.
			next_token();

			NodePin<ClassNode> class_node;

			if (!(class_node = make_node<ClassNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(class_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			class_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!class_node->set_name(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p_scope->push_member(class_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::Deferred set_token_range_guard([this, token, class_node]() noexcept {
					class_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
				});

				NodePin<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = class_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(
					parse_generic_params(
						allocator,
						class_node->get_scope()->generic_params,
						class_node->sti_generic_params_comma_separators,
						class_node->sti_generic_left_angle,
						class_node->sti_generic_right_angle));

				if (Token *l_parenthese_token = peek_token(); l_parenthese_token->token_id == TokenId::LParenthese) {
					next_token();

					TypeName inherited_type;
					SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, inherited_type));
					class_node->get_scope()->inherited_type = inherited_type;

					Token *r_parenthese_token;
					SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_parenthese_token = peek_token()), TokenId::RParenthese)));

					next_token();
				}

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						ImplementItem item;

						item.is_trait = false;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, item.type));

						if (!class_node->get_scope()->implemented_types.push_back(std::move(item))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(allocator)(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p_scope->members_index.find(class_node->get_name()); it != p_scope->members_index.end()) {
				peff::String s(allocator);

				if (!s.build(class_node->get_name())) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(class_node->get_token_range(), std::move(ex_data));
			} else {
				if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}
			}

			break;
		}
		case TokenId::StructKeyword: {
			// Struct.
			next_token();

			NodePin<StructNode> struct_node;

			if (!(struct_node = make_node<StructNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(struct_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			struct_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!struct_node->set_name(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p_scope->push_member(struct_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			{
				peff::Deferred set_token_range_guard([this, token, struct_node]() noexcept {
					struct_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
				});

				NodePin<MemberNode> prev_parent;
				prev_parent = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_parent]() noexcept {
					cur_parent = prev_parent;
				});
				cur_parent = struct_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_generic_params(
					allocator,
					struct_node->get_scope()->generic_params,
					struct_node->sti_generic_params_comma_separators,
					struct_node->sti_generic_left_angle,
					struct_node->sti_generic_right_angle));

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						ImplementItem item;

						item.is_trait = false;

						TypeName tn;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, tn));

						if (!struct_node->get_scope()->implemented_types.push_back(std::move(item))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(allocator)(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p_scope->members_index.find(struct_node->get_name()); it != p_scope->members_index.end()) {
				peff::String s(allocator);

				if (!s.build(struct_node->get_name())) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(struct_node->get_token_range(), std::move(ex_data));
			} else {
				if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}
			}

			break;
		}
		case TokenId::InterfaceKeyword: {
			// Interface.
			next_token();

			NodePin<InterfaceNode> interface_node;

			if (!(interface_node = make_node<InterfaceNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			if (!(interface_node->alloc_scope()))
				co_return gen_oom_syntax_error();

			interface_node->access_modifier = access;

			Token *name_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

			next_token();

			if (!interface_node->set_name(name_token->source_text)) {
				co_return gen_oom_syntax_error();
			}

			size_t idx_member;
			if ((idx_member = p_scope->push_member(interface_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			Token *t;

			{
				peff::Deferred set_token_range_guard([this, token, interface_node]() noexcept {
					interface_node->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
				});

				NodePin<MemberNode> prev_member;
				prev_member = cur_parent;
				peff::ScopeGuard restore_prev_mod_guard([this, prev_member]() noexcept {
					cur_parent = prev_member;
				});
				cur_parent = interface_node.cast_to<MemberNode>();

				SLKC_CO_RETURN_IF_CO_PARSE_ERROR(
					parse_generic_params(
						allocator,
						interface_node->get_scope()->generic_params,
						interface_node->sti_generic_params_comma_separators,
						interface_node->sti_generic_left_angle,
						interface_node->sti_generic_right_angle));

				if (Token *colon_token = peek_token(); colon_token->token_id == TokenId::Colon) {
					next_token();

					while (true) {
						ImplementItem item;

						item.is_trait = false;

						SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_type_name(allocator, item.type));

						if (!interface_node->get_scope()->implemented_types.push_back(std::move(item))) {
							co_return gen_oom_syntax_error();
						}

						if (peek_token()->token_id != TokenId::AddOp) {
							break;
						}

						Token *or_op_token = next_token();
					}
				}

				Token *l_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((l_brace_token = peek_token()), TokenId::LBrace)));

				next_token();

				Token *current_token;
				while (true) {
					SLKC_CO_RETURN_IF_PARSE_ERROR(expect_token(current_token = peek_token()));

					if (current_token->token_id == TokenId::RBrace) {
						break;
					}

					if ((syntax_error = (co_await parse_program_stmt(allocator)(this)))) {
						// Parse the rest to make sure that we have gained all of the information,
						// instead of ignoring them.
						if (!syntax_errors.push_back(std::move(syntax_error.value())))
							co_return gen_oom_syntax_error();
						syntax_error.reset();
					}
				}

				Token *r_brace_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((r_brace_token = peek_token()), TokenId::RBrace)));

				next_token();
			}

			if (auto it = p_scope->members_index.find(interface_node->get_name()); it != p_scope->members_index.end()) {
				peff::String s(allocator);

				if (!s.build(interface_node->get_name())) {
					co_return gen_oom_syntax_error();
				}

				ConflictingDefinitionsErrorExData ex_data(std::move(s));

				co_return SyntaxError(interface_node->get_token_range(), std::move(ex_data));
			} else {
				if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}
			}

			break;
		}
		case TokenId::ImportKeyword: {
			// Import item.
			next_token();

			NodePin<ImportNode> import_node;

			if (!(import_node = make_node<ImportNode>(get_global()))) {
				co_return gen_oom_syntax_error();
			}

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_id_ref(allocator, import_node->id_ref));
			size_t idx_member;
			if ((idx_member = p_scope->push_member(import_node.cast_to<MemberNode>())) == SIZE_MAX) {
				co_return gen_oom_syntax_error();
			}

			if (Token *as_token = peek_token(); as_token->token_id == TokenId::AsKeyword) {
				next_token();

				Token *name_token;

				SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((name_token = peek_token()), TokenId::Id)));

				if (!import_node->set_name(name_token->source_text)) {
					co_return gen_oom_syntax_error();
				}

				if (auto result = p_scope->index_member(idx_member); result != ScopeMemberOpResult::Success) {
					co_return scope_member_op_result_to_syntax_error(result);
				}
			} else {
				if (!p_scope->anonymous_imports.push_back(NodePin<ImportNode>(import_node))) {
					co_return gen_oom_syntax_error();
				}
			}

			Token *semicolon_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

			next_token();

			break;
		}
		case TokenId::LetKeyword: {
			// Global variable.
			next_token();

			NodePin<VarDefStmtNode> stmt;

			if (!(stmt = make_node<VarDefStmtNode>(
					  get_global()))) {
				co_return gen_oom_syntax_error();
			}

			// TODO: Use them.
			/*stmt->access_modifier = access;

			if (!p->var_def_stmts.push_back(NodePin<VarDefStmtNode>(stmt))) {
				co_return gen_oom_syntax_error();
			}*/

			peff::Deferred set_token_range_guard([this, token, stmt]() noexcept {
				stmt->set_token_range(TokenRange{ parse_context.mod, token->index, parse_context.idx_prev_token });
			});

			SLKC_CO_RETURN_IF_CO_PARSE_ERROR(parse_var_defs(allocator, stmt->bindings));

			Token *semicolon_token;

			SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

			next_token();

			for (auto &i : stmt->bindings) {
				if (p_scope->members_index.contains(i.name)) {
					peff::String s(allocator);

					if (!s.build(i.name))
						co_return gen_oom_syntax_error();

					ConflictingDefinitionsErrorExData ex_data(std::move(s));

					if (syntax_errors.push_back(SyntaxError(TokenRange(parse_context.mod, i.sti_name_token), std::move(ex_data))))
						co_return gen_oom_syntax_error();
				}
				// TODO: Fix up and use them.
				/*
				NodePin<VarNode> var_node;

				if (!(var_node = make_node<VarNode>(get_global()))) {
					co_return gen_oom_syntax_error();
				}

				if (!var_node->set_name(i->name))
					co_return gen_oom_syntax_error();
				var_node->initial_value = i->initial_value;
				var_node->type = i->type;
				var_node->access_modifier = stmt->access_modifier;

				if (!p_scope->add_member(var_node.cast_to<MemberNode>()))
					co_return gen_oom_syntax_error();*/
			}

			break;
		}
		default:
			next_token();
			co_return SyntaxError(
				TokenRange{ parse_context.mod, token->index },
				SyntaxErrorKind::ExpectingDecl);
	}

	co_return peff::NULLOPT;
}

SLKC_API ParseCoroutine Parser::parse_program(peff::Alloc *allocator, const NodePin<ModuleNode> &initial_mod, OwnedIdRef &module_name_out) {
	peff::Option<SyntaxError> syntax_error;

	Token *t;

	parse_context.mod = initial_mod;
	cur_parent = initial_mod.cast_to<MemberNode>();

	if ((t = peek_token())->token_id == TokenId::ModuleKeyword) {
		next_token();

		if ((syntax_error = (co_await parse_id_ref(allocator, module_name_out)(this)))) {
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
		}

		Token *semicolon_token;
		SLKC_CO_RETURN_IF_PARSE_ERROR((expect_token((semicolon_token = peek_token()), TokenId::Semicolon)));

		next_token();
	}

	while ((t = peek_token())->token_id != TokenId::End) {
		if ((syntax_error = (co_await parse_program_stmt(allocator)(this)))) {
			// Parse the rest to make sure that we have gained all of the information,
			// instead of ignoring them.
			if (!syntax_errors.push_back(std::move(syntax_error.value())))
				co_return gen_oom_syntax_error();
			syntax_error.reset();
		}
	}

	initial_mod->module_source_token_list = std::move(token_list);

	co_return peff::NULLOPT;
}

SLKC_API peff::Option<SyntaxError> Parser::parse(const NodePin<ModuleNode> &initial_mod, OwnedIdRef &module_name_out) {
	return parse_program(global->get_allocator(), initial_mod, module_name_out).resume(this);
}