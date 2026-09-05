#define NOMINMAX
#include "rg2ast.h"

using namespace slkc;
using namespace slkc::comp;

SLAKE_API peff::Option<CompilationError> CompilationCoroutine::resume(CompilationCoroutineScheduler *scheduler) {
	if (!coro_handle)
		return CompilationError(CompilationErrorKind::OutOfMemory);

	coro_handle.resume();

	while (scheduler->task_list.size()) {
		auto h = scheduler->task_list.back();
		scheduler->task_list.pop_back();
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

SLKC_API CompilationCoroutine::Awaitable::Awaitable(
	CompilationCoroutine &co,
	CompilationCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLKC_API bool CompilationCoroutine::Awaitable::await_ready() {
	return false;
}

SLKC_API void CompilationCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = CompilationError(CompilationErrorKind::OutOfMemory);
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = CompilationError(CompilationErrorKind::OutOfMemory);
		return;
	}
}

SLKC_API peff::Option<CompilationError> CompilationCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result)
			return std::move(handle.promise().result);
		return peff::NULLOPT;
	}
	return CompilationError(CompilationErrorKind::OutOfMemory);
}

SLKC_API CompilationCoroutine::Awaitable CompilationCoroutine::operator()(CompilationCoroutineScheduler *scheduler) {
	return Awaitable(*this, scheduler, coro_handle);
}

SLKC_API CompilationCoroutineScheduler::CompilationCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

SLKC_API peff::Option<CompilationError> comp::_pin_fail_reason_to_comp_error(ast::PinFailReason reason) {
	switch (reason) {
		case ast::PinFailReason::IOError:
			return gen_pinning_io_error_option();
		case ast::PinFailReason::OutOfMemory:
			return gen_oom_error_option();
		case ast::PinFailReason::OutOfNodeIndex:
			return gen_out_of_node_index_error_option();
		default:
			std::terminate();
	}
}

SLKC_API peff::Option<CompilationError> comp::_green_node_op_result_to_comp_error(ast::GreenNodeOperationResult result) {
	switch (result) {
		case ast::GreenNodeOperationResult::Success:
			return peff::NULLOPT;
		case ast::GreenNodeOperationResult::PinIOError:
			return gen_pinning_io_error_option();
		case ast::GreenNodeOperationResult::OutOfMemory:
			return gen_oom_error_option();
		case ast::GreenNodeOperationResult::OutOfNodeIndex:
			return gen_out_of_node_index_error_option();
	}
	std::terminate();
}

template <typename T>
SLAKE_FORCEINLINE static peff::Option<CompilationError> _parse_int(
	CompilationEnv *env,
	const ast::TokenPtr &token,
	bool is_negative,
	const std::string_view &body_view,
	T &data_out) {
	peff::Option<CompilationError> syntax_error;

	bool overflow_warned = false;
	char c;
	T data = 0;
	size_t i = (size_t)is_negative;

	auto push_overflowed_error = [env, &token]() noexcept -> peff::Option<CompilationError> {
		SLKC_RETURN_IF_COMP_ERROR(
			env->push_error(
				CompilationError(
					ast::TokenRange{ env->get_target_module().get_index(), token->index }, CompilationErrorKind::LiteralOverflowed)));
		return peff::NULLOPT;
	};

	switch (((ast::IntTokenExtension *)token->ex_data.get())->token_type) {
		case ast::IntTokenType::Decimal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 10 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 10;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 10;
					data += c - '0';
				}
				++i;
			}
			break;
		case ast::IntTokenType::Hexadecimal:
			while (i < body_view.size()) {
				char c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 16 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data -= c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data -= c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data -= c - 'A' + 10;
							break;
					}
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data += c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data += c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data += c - 'A' + 10;
							break;
					}
				}
				++i;
			}
			break;
		case ast::IntTokenType::Octal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 8 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 8;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 8 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 8;
					data += c - '0';
				}
				++i;
			}
			break;
		case ast::IntTokenType::Binary:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && ((std::numeric_limits<T>::min() >> 1) > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data <<= 1;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && ((std::numeric_limits<T>::max() >> 1) < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data <<= 1;
					data += c - '0';
				}
				++i;
			}
			break;
		default:
			std::terminate();
	}

	data_out = data;

	return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::TypeName &type_name_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	auto g = red_node->as_green_node();
	assert(g->node_kind == ast::GreenNodeKind::TypeName);

	auto exdata = std::get<ast::TypeNameGreenNodeExData>(g->exdata);

	// TODO: Implement it.

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_id_ref(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::OwnedIdRef &id_ref_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	auto g = red_node->as_green_node();
	assert(g->node_kind == ast::GreenNodeKind::IdRef);

	for (auto i : indices.get_classified_indices(ast::GreenNodeKind::IdRefEntry)) {
		ast::RedNodePtr entry_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				red_node->get_child_node(state_allocator, i, entry_node)));

		assert(entry_node->is_green_node_facade());

		ast::IdRefEntry ast_entry(env->get_global()->get_allocator());

		ast::RedNodeChildIndices entry_indices(state_allocator);

		if (!entry_indices.index_children(entry_node))
			co_return gen_oom_error_option();

		ast::RedNodePtr id_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				entry_node->get_child_node(state_allocator, entry_indices.get_classified_indices(ast::TokenId::Id)[0], id_node)));

		ast_entry.name = id_node->as_token()->source_text;

		for (auto j : entry_indices.get_classified_indices(ast::GreenNodeKind::TypeName)) {
			ast::RedNodePtr tn_node;
			SLKC_CO_RETURN_IF_COMP_ERROR(
				_green_node_op_result_to_comp_error(
					entry_node->get_child_node(state_allocator, entry_indices.get_classified_indices(ast::GreenNodeKind::TypeName)[j], tn_node)));

			ast::TypeName tn;

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, tn_node, tn)(sched));

			if (!ast_entry.generic_args.push_back(std::move(tn)))
				co_return gen_oom_error_option();
		}

		if (!id_ref_out.entries.push_back(std::move(ast_entry)))
			co_return gen_oom_error_option();
	}

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::AstNodePin<ast::AstNode> &ast_node_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	// TODO: Implement it.
	auto g = red_node->as_green_node();
	switch (g->node_kind) {
		case ast::GreenNodeKind::Expr: {
			auto exdata = std::get<ast::ExprGreenNodeExData>(g->exdata);

			// TODO: Give anchors of tokens to the AST nodes.
			switch (exdata.expr_kind) {
				case slkc::ast::GreenNodeExprKind::I8Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I8LiteralExprNode> e = ast::make_ast_node<ast::I8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I16Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I16Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I16LiteralExprNode> e = ast::make_ast_node<ast::I16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I32LiteralExprNode> e = ast::make_ast_node<ast::I32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I64LiteralExprNode> e = ast::make_ast_node<ast::I64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U8Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U8LiteralExprNode> e = ast::make_ast_node<ast::U8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U16Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U16LiteralExprNode> e = ast::make_ast_node<ast::U16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U32LiteralExprNode> e = ast::make_ast_node<ast::U32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U64LiteralExprNode> e = ast::make_ast_node<ast::U64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					float literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F32LiteralExprNode> e = ast::make_ast_node<ast::F32LiteralExprNode>(env->get_global(), strtof(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					double literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F64LiteralExprNode> e = ast::make_ast_node<ast::F64LiteralExprNode>(env->get_global(), strtod(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::StringLiteral: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::StringLiteralExprNode> e = ast::make_ast_node<ast::StringLiteralExprNode>(env->get_global(), static_cast<ast::StringTokenExtension *>(t->ex_data.get())->data);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::BoolLiteral: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::BoolLiteralExprNode> e = ast::make_ast_node<ast::BoolLiteralExprNode>(env->get_global(), t->token_id == ast::TokenId::TrueKeyword ? true : false);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::NullLiteral: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::NullLiteralExprNode> e = ast::make_ast_node<ast::NullLiteralExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Unary: {
					ast::UnaryOp op;

					switch (exdata.unary_expr_op) {
						case ast::GreenNodeUnaryExprOp::LNot:
							op = ast::UnaryOp::LNot;
							break;
						case ast::GreenNodeUnaryExprOp::Not:
							op = ast::UnaryOp::Not;
							break;
						case ast::GreenNodeUnaryExprOp::Neg:
							op = ast::UnaryOp::Neg;
							break;
						case ast::GreenNodeUnaryExprOp::Move:
							op = ast::UnaryOp::Move;
							break;
						case ast::GreenNodeUnaryExprOp::Unpacking:
							op = ast::UnaryOp::Unpacking;
							break;
						default:
							std::terminate();
					}

					ast::AstNodePin<ast::UnaryExprNode> e = ast::make_ast_node<ast::UnaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					e->unary_op = op;

					ast::RedNodePtr operand_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], operand_node)));

					ast::AstNodePin<ast::AstNode> operand;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, operand_node, operand)(sched));

					e->operand = operand.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::Binary: {
					ast::BinaryOp op;

					switch (exdata.binary_expr_op) {
						case ast::GreenNodeBinaryExprOp::Add:
							op = ast::BinaryOp::Add;
							break;
						case ast::GreenNodeBinaryExprOp::Sub:
							op = ast::BinaryOp::Sub;
							break;
						case ast::GreenNodeBinaryExprOp::Mul:
							op = ast::BinaryOp::Mul;
							break;
						case ast::GreenNodeBinaryExprOp::Div:
							op = ast::BinaryOp::Div;
							break;
						case ast::GreenNodeBinaryExprOp::Mod:
							op = ast::BinaryOp::Mod;
							break;
						case ast::GreenNodeBinaryExprOp::And:
							op = ast::BinaryOp::And;
							break;
						case ast::GreenNodeBinaryExprOp::Or:
							op = ast::BinaryOp::Or;
							break;
						case ast::GreenNodeBinaryExprOp::Xor:
							op = ast::BinaryOp::Xor;
							break;
						case ast::GreenNodeBinaryExprOp::LAnd:
							op = ast::BinaryOp::LAnd;
							break;
						case ast::GreenNodeBinaryExprOp::LOr:
							op = ast::BinaryOp::LOr;
							break;
						case ast::GreenNodeBinaryExprOp::Shl:
							op = ast::BinaryOp::Shl;
							break;
						case ast::GreenNodeBinaryExprOp::Shr:
							op = ast::BinaryOp::Shr;
							break;
						case ast::GreenNodeBinaryExprOp::Assign:
							op = ast::BinaryOp::Assign;
							break;
						case ast::GreenNodeBinaryExprOp::AddAssign:
							op = ast::BinaryOp::AddAssign;
							break;
						case ast::GreenNodeBinaryExprOp::SubAssign:
							op = ast::BinaryOp::SubAssign;
							break;
						case ast::GreenNodeBinaryExprOp::MulAssign:
							op = ast::BinaryOp::MulAssign;
							break;
						case ast::GreenNodeBinaryExprOp::DivAssign:
							op = ast::BinaryOp::DivAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ModAssign:
							op = ast::BinaryOp::ModAssign;
							break;
						case ast::GreenNodeBinaryExprOp::AndAssign:
							op = ast::BinaryOp::AndAssign;
							break;
						case ast::GreenNodeBinaryExprOp::OrAssign:
							op = ast::BinaryOp::OrAssign;
							break;
						case ast::GreenNodeBinaryExprOp::XorAssign:
							op = ast::BinaryOp::XorAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ShlAssign:
							op = ast::BinaryOp::ShlAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ShrAssign:
							op = ast::BinaryOp::ShrAssign;
							break;
						case ast::GreenNodeBinaryExprOp::Eq:
							op = ast::BinaryOp::Eq;
							break;
						case ast::GreenNodeBinaryExprOp::Neq:
							op = ast::BinaryOp::Neq;
							break;
						case ast::GreenNodeBinaryExprOp::PhyEq:
							op = ast::BinaryOp::PhyEq;
							break;
						case ast::GreenNodeBinaryExprOp::PhyNeq:
							op = ast::BinaryOp::PhyNeq;
							break;
						case ast::GreenNodeBinaryExprOp::Lt:
							op = ast::BinaryOp::Lt;
							break;
						case ast::GreenNodeBinaryExprOp::Gt:
							op = ast::BinaryOp::Gt;
							break;
						case ast::GreenNodeBinaryExprOp::LtEq:
							op = ast::BinaryOp::LtEq;
							break;
						case ast::GreenNodeBinaryExprOp::GtEq:
							op = ast::BinaryOp::GtEq;
							break;
						case ast::GreenNodeBinaryExprOp::Cmp:
							op = ast::BinaryOp::Cmp;
							break;
						default:
							std::terminate();
					}

					ast::AstNodePin<ast::BinaryExprNode> e = ast::make_ast_node<ast::BinaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					e->binary_op = op;

					ast::RedNodePtr lhs_node, rhs_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], lhs_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], rhs_node)));

					ast::AstNodePin<ast::AstNode> lhs, rhs;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, lhs_node, lhs)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, rhs_node, rhs)(sched));

					e->lhs = lhs.cast_to<ast::ExprNode>();
					e->rhs = rhs.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::Ternary: {
					ast::AstNodePin<ast::TernaryExprNode> e = ast::make_ast_node<ast::TernaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast::RedNodePtr cond_node, true_node, false_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], cond_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], true_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[2], false_node)));

					ast::AstNodePin<ast::AstNode> cond, tb, fb;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, cond_node, cond)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_node, tb)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, false_node, fb)(sched));

					e->condition = cond.cast_to<ast::ExprNode>();
					e->true_branch = tb.cast_to<ast::ExprNode>();
					e->false_branch = fb.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::IdRef: {
					ast::AstNodePin<ast::IdRefExprNode> e = ast::make_ast_node<ast::IdRefExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast::RedNodePtr id_ref_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::IdRef)[0], id_ref_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_id_ref(state_allocator, sched, env, id_ref_node, e->id_ref)(sched));

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::HeadedIdRef: {
					ast::AstNodePin<ast::HeadedIdRefExprNode> e = ast::make_ast_node<ast::HeadedIdRefExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], expr_node)));

						ast::AstNodePin<ast::AstNode> head_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, head_expr)(sched));
						e->head_expr = std::move(head_expr).cast_to<ast::ExprNode>();
					}

					ast::RedNodePtr id_ref_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::IdRef)[0], id_ref_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_id_ref(state_allocator, sched, env, id_ref_node, e->id_ref)(sched));

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::InitializerList: {
					ast::AstNodePin<ast::InitializerListExprNode> e = ast::make_ast_node<ast::InitializerListExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(args_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (e->elements.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Call: {
					ast::AstNodePin<ast::CallExprNode> e = ast::make_ast_node<ast::CallExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr callee_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], callee_node)));

						ast::AstNodePin<ast::AstNode> callee_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, callee_node, callee_expr)(sched));
						e->target = std::move(callee_expr).cast_to<ast::ExprNode>();
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(args_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Subscript: {
					ast::AstNodePin<ast::SubscriptExprNode> e = ast::make_ast_node<ast::SubscriptExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr callee_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], callee_node)));

						ast::AstNodePin<ast::AstNode> callee_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, callee_node, callee_expr)(sched));
						e->target = std::move(callee_expr).cast_to<ast::ExprNode>();
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(args_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::New: {
					ast::AstNodePin<ast::NewExprNode> e = ast::make_ast_node<ast::NewExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr new_type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], new_type_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, new_type_node, e->target_type)(sched));
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(args_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Cast: {
					ast::AstNodePin<ast::CastExprNode> e = ast::make_ast_node<ast::CastExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], type_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, type_node, e->target_type)(sched));
					}

					{
						ast::RedNodePtr operand_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], operand_node)));

						ast::AstNodePin<ast::AstNode> operand;

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, operand_node, operand)(sched));

						e->operand = operand.cast_to<ast::ExprNode>();
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Match: {
					ast::AstNodePin<ast::MatchExprNode> e = ast::make_ast_node<ast::MatchExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					// Convert the condition node.
					{
						ast::RedNodePtr condition_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], condition_node)));

						ast::AstNodePin<ast::AstNode> condition;

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, condition_node, condition)(sched));

						e->condition = condition.cast_to<ast::ExprNode>();
					}

					// Check if there is explicit return type.
					if (auto index = indices.get_classified_indices(ast::GreenNodeKind::TypeName); index.size()) {
						ast::RedNodePtr type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], type_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, type_node, e->return_type)(sched));
					}

					// Lowering the branches.
					{
						ast::RedNodePtr branches_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::MatchCase)[0], branches_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(branches_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr case_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, case_node)));

							assert(case_node->is_green_node_facade());

							ast::RedNodePtr pattern_node, result_value_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], pattern_node)));
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], result_value_node)));

							ast::MatchExprBranch branch;

							ast::AstNodePin<ast::AstNode> pattern, result_value;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, pattern_node, pattern)(sched));
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_value_node, result_value)(sched));

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Break: {
					ast::AstNodePin<ast::BreakExprNode> e = ast::make_ast_node<ast::BreakExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					// TODO: Add label support if implemented.

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Continue: {
					ast::AstNodePin<ast::ContinueExprNode> e = ast::make_ast_node<ast::ContinueExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices indices(state_allocator);

						assert(args_node->is_green_node_facade());
						for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									red_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (e->continue_values.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

							break;
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Return: {
					ast::AstNodePin<ast::ReturnExprNode> e = ast::make_ast_node<ast::ReturnExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr result_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], result_node)));

						ast::AstNodePin<ast::AstNode> result_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_node, result_expr)(sched));
						e->return_value = std::move(result_expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Yield: {
					ast::AstNodePin<ast::YieldExprNode> e = ast::make_ast_node<ast::YieldExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr result_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], result_node)));

						ast::AstNodePin<ast::AstNode> result_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_node, result_expr)(sched));
						e->return_value = std::move(result_expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Group: {
					ast::AstNodePin<ast::GroupExprNode> e = ast::make_ast_node<ast::GroupExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], expr_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
						e->operand = std::move(expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
	}

	co_return peff::NULLOPT;
}

SLKC_API peff::Result<ast::AstNodePin<ast::AstNode>, CompilationError> comp::lower_rg_node_to_ast_node(peff::Alloc *state_allocator, comp::CompilationEnv *env, const PEFF_IN_REF ast::RedNodePtr &green_node) {
	ast::AstNodePin<ast::AstNode> node;

	CompilationCoroutineScheduler sched(state_allocator);

	SLKC_RETURN_IF_COMP_ERROR(_do_lower_rg_node_to_ast_node(state_allocator, &sched, env, green_node, node).resume(&sched));

	return node;
}
