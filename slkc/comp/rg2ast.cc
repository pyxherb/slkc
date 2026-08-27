#include "rg2ast.h"

using namespace slkc;
using namespace slkc::comp;

SLAKE_API peff::Option<CompilationError> RGLoweringCoroutine::resume() {
	if (!coro_handle)
		return CompilationError(CompilationErrorKind::OutOfMemory);

	coro_handle.resume();

	while (this->coro_handle.promise().scheduler->task_list.size()) {
		auto h = this->coro_handle.promise().scheduler->task_list.back();
		this->coro_handle.promise().scheduler->task_list.pop_back();
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
