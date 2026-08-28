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

SLKC_API RGLoweringCoroutine comp::_do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, const ast::RedNodePtr red_node, PEFF_OUT_REF ast::AstNodePtr<ast::AstNode> &ast_node_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if(!indices.index_children(red_node))
		co_return gen_oom_error_option();

	// TODO: Implement it.
	switch(red_node->as_green_node()->node_kind) {}

	co_return peff::NULLOPT;
}

SLKC_API peff::Result<ast::AstNodePtr<ast::AstNode>, CompilationError> comp::lower_rg_node_to_ast_node(peff::Alloc *state_allocator, const ast::RedNodePtr green_node) {
	ast::AstNodePtr<ast::AstNode> node;
	auto co = _do_lower_rg_node_to_ast_node(state_allocator, green_node, node);

	RGLoweringCoroutineScheduler sched(state_allocator);

	SLKC_RETURN_IF_COMP_ERROR(co.resume(&sched));

	return node;
}
