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

SLKC_API  void ParseCoroutine::Awaitable::await_suspend(Handle h) {
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
