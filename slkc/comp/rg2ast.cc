#define NOMINMAX
#include "rg2ast.h"

using namespace slkc;
using namespace slkc::comp;

SLAKE_API peff::Option<CompilationError> RGLoweringCoroutine::resume(RGLoweringCoroutineScheduler *scheduler) {
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

SLKC_API RGLoweringCoroutine::Awaitable::Awaitable(
	RGLoweringCoroutine &co,
	RGLoweringCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLKC_API bool RGLoweringCoroutine::Awaitable::await_ready() {
	return false;
}

SLKC_API void RGLoweringCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = CompilationError(CompilationErrorKind::OutOfMemory);
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = CompilationError(CompilationErrorKind::OutOfMemory);
		return;
	}
}

SLKC_API peff::Option<CompilationError> RGLoweringCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result)
			return std::move(handle.promise().result);
		return peff::NULLOPT;
	}
	return CompilationError(CompilationErrorKind::OutOfMemory);
}

SLKC_API RGLoweringCoroutine::Awaitable RGLoweringCoroutine::operator()(RGLoweringCoroutineScheduler *scheduler) {
	return Awaitable(*this, scheduler, coro_handle);
}

SLKC_API RGLoweringCoroutineScheduler::RGLoweringCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
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

SLKC_API RGLoweringCoroutine comp::_do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, PEFF_OUT_REF ast::AstNodePtr<ast::AstNode> &ast_node_out) {
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
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal).front()).value();
					auto t = literal_node->as_token();

					int8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I8LiteralExprNode> e = ast::make_ast_node<ast::I8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I16Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I16Literal).front()).value();
					auto t = literal_node->as_token();

					int16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I16LiteralExprNode> e = ast::make_ast_node<ast::I16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I32Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I32Literal).front()).value();
					auto t = literal_node->as_token();

					int32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I32LiteralExprNode> e = ast::make_ast_node<ast::I32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I64Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I64Literal).front()).value();
					auto t = literal_node->as_token();

					int64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I64LiteralExprNode> e = ast::make_ast_node<ast::I64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U8Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U8Literal).front()).value();
					auto t = literal_node->as_token();

					uint8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U8LiteralExprNode> e = ast::make_ast_node<ast::U8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U16Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U16Literal).front()).value();
					auto t = literal_node->as_token();

					uint16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U16LiteralExprNode> e = ast::make_ast_node<ast::U16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U32Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U32Literal).front()).value();
					auto t = literal_node->as_token();

					uint32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U32LiteralExprNode> e = ast::make_ast_node<ast::U32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U64Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal).front()).value();
					auto t = literal_node->as_token();

					uint64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U64LiteralExprNode> e = ast::make_ast_node<ast::U64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F32Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U32Literal).front()).value();
					auto t = literal_node->as_token();

					float literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F32LiteralExprNode> e = ast::make_ast_node<ast::F32LiteralExprNode>(env->get_global(), strtof(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F64Literal: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal).front()).value();
					auto t = literal_node->as_token();

					double literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F64LiteralExprNode> e = ast::make_ast_node<ast::F64LiteralExprNode>(env->get_global(), strtod(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::StringLiteral: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal).front()).value();
					auto t = literal_node->as_token();

					ast::AstNodePin<ast::StringLiteralExprNode> e = ast::make_ast_node<ast::StringLiteralExprNode>(env->get_global(), static_cast<ast::StringTokenExtension *>(t->ex_data.get())->data);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::BoolLiteral: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal).front()).value();
					auto t = literal_node->as_token();

					ast::AstNodePin<ast::BoolLiteralExprNode> e = ast::make_ast_node<ast::BoolLiteralExprNode>(env->get_global(), t->token_id == ast::TokenId::TrueKeyword ? true : false);

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::NullLiteral: {
					ast::RedNodePtr literal_node = red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal).front()).value();
					auto t = literal_node->as_token();

					ast::AstNodePin<ast::NullLiteralExprNode> e = ast::make_ast_node<ast::NullLiteralExprNode>(env->get_global());

					if (!e)
						co_return gen_oom_error_option();

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Unary: {
					auto op = exdata.unary_expr_op;
					break;
				}
				case slkc::ast::GreenNodeExprKind::Binary: {
					auto op = exdata.binary_expr_op;
					break;
				}
			}
			break;
		}
	}

	co_return peff::NULLOPT;
}

SLKC_API peff::Result<ast::AstNodePtr<ast::AstNode>, CompilationError> comp::lower_rg_node_to_ast_node(peff::Alloc *state_allocator, comp::CompilationEnv *env, const PEFF_IN_REF ast::RedNodePtr &green_node) {
	ast::AstNodePtr<ast::AstNode> node;
	auto co = _do_lower_rg_node_to_ast_node(state_allocator, env, green_node, node);

	RGLoweringCoroutineScheduler sched(state_allocator);

	SLKC_RETURN_IF_COMP_ERROR(co.resume(&sched));

	return node;
}
