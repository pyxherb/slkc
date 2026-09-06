#ifndef _SLKC_COMP_RG2AST_H_
#define _SLKC_COMP_RG2AST_H_

#include "env.h"
#include <coroutine>

namespace slkc {
	namespace comp {
		struct CompilationCoroutineScheduler;

		struct CompilationCoroutine {
			struct promise_type;

			using Handle = std::coroutine_handle<promise_type>;

			struct promise_type {
				peff::Option<CompilationError> result;

				SLAKE_FORCEINLINE static CompilationCoroutine get_return_object_on_allocation_failure() noexcept {
					return CompilationCoroutine({});
				}

				SLAKE_FORCEINLINE CompilationCoroutine get_return_object() noexcept {
					return CompilationCoroutine(Handle::from_promise(*this));
				}

				SLAKE_FORCEINLINE std::suspend_always initial_suspend() noexcept {
					return {};
				}

				SLAKE_FORCEINLINE std::suspend_always final_suspend() noexcept {
					return {};
				}

				SLAKE_FORCEINLINE std::suspend_always yield_value(peff::Option<CompilationError> &&value) noexcept {
					result = std::move(value);
					return {};
				}

				SLAKE_FORCEINLINE void return_value(peff::Option<CompilationError> &&value) noexcept {
					result = std::move(value);
				}

				SLAKE_FORCEINLINE void unhandled_exception() { std::terminate(); }

				struct AllocatorInfo {
					peff::Alloc *allocator;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					size_t c;
#endif
				};

				template <typename... Args>
				SLAKE_FORCEINLINE static void *operator new(size_t size, peff::Alloc *allocator, Args &&...args) noexcept {
					char *p = (char *)allocator->alloc(size + sizeof(AllocatorInfo), alignof(std::max_align_t));

					if (!p)
						return nullptr;

					memset(p, 0, size);

#if PEFF_ENABLE_RCOBJ_DEBUGGING
					auto ref_count = peff::acquire_global_rcobj_ptr_counter();
#endif

					AllocatorInfo allocator_info = {
						allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
						,
						ref_count
#endif
					};

					memcpy(p + size, &allocator_info, sizeof(allocator_info));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					allocator->inc_ref(ref_count);
#endif

					return p;
				}

				SLAKE_FORCEINLINE static void operator delete(void *p, size_t size) noexcept {
					AllocatorInfo allocator_info;

					memcpy(&allocator_info, (char *)p + size, sizeof(allocator_info));

					peff::RcObjectPtr<peff::Alloc> allocator_holder = allocator_info.allocator;

					allocator_holder->release(p, size + sizeof(AllocatorInfo), alignof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					allocator_holder->dec_ref(allocator_info.c);
#endif
				}
			};

			Handle coro_handle;

			static inline bool recursed = false;

			CompilationCoroutine(Handle coro_handle) : coro_handle(coro_handle) {}
			~CompilationCoroutine() {
				// assert(!recursed);
				// recursed = true;
				if (coro_handle)
					coro_handle.destroy();
				// recursed = false;
			}

			SLAKE_FORCEINLINE bool done() {
				return coro_handle.done();
			}

			SLAKE_API peff::Option<CompilationError> resume(CompilationCoroutineScheduler *scheduler);

			struct Awaitable {
				CompilationCoroutine &co;
				CompilationCoroutineScheduler *scheduler;
				Handle handle;

				SLKC_API Awaitable(CompilationCoroutine &co, CompilationCoroutineScheduler *scheduler, Handle handle);
				SLKC_API bool await_ready();
				SLKC_API void await_suspend(Handle h);
				[[nodiscard]] SLKC_API peff::Option<CompilationError> await_resume();
			};

			SLKC_API Awaitable operator()(CompilationCoroutineScheduler *scheduler);
		};

		class CompilationCoroutineScheduler {
		public:
			peff::DynArray<std::coroutine_handle<CompilationCoroutine::promise_type>> task_list;

			SLKC_API CompilationCoroutineScheduler(peff::Alloc *allocator);
		};

		SLKC_API peff::Option<CompilationError> _pin_fail_reason_to_comp_error(ast::PinFailReason reason);
		SLKC_API peff::Option<CompilationError> _green_node_op_result_to_comp_error(ast::GreenNodeOperationResult result);

		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_var_binding(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::BindingEntry &binding_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::TypeName &type_name_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_id_ref(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::OwnedIdRef &id_ref_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::AstNodePin<ast::AstNode> &ast_node_out);
		SLKC_API peff::Result<ast::AstNodePin<ast::AstNode>, CompilationError> lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node);
	}
}

#endif
