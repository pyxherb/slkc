#ifndef _SLKC_COMP_ENV_H_
#define _SLKC_COMP_ENV_H_

#include <slkc/ast/utils.h>
#include <slkc/ast/rgtree.h>
#include <slkc/ast/nodedefs.h>

namespace slkc {
	namespace comp {
		enum class CompilationErrorKind : uint32_t {
			OutOfMemory = 0,
			StackOverflow,
			OutOfRuntimeMemory,
			PinningIOError,
			OutOfNodeIndex,

			ExpectingRValueExpr,
			TargetIsNotCallable,
			TargetIsNotUnpackable,
			NoSuchFnOverloading,
			IncompatibleOperand,
			OperatorNotFound,
			AmbiguousOperatorCall,
			MismatchedGenericArgNumber,
			ExpectingTypeName,
			ExpectingClassName,
			ExpectingInterfaceName,
			AbstractMethodNotImplemented,
			CyclicInheritedClass,
			CyclicInheritedInterface,
			RecursedValueType,
			ExpectingId,
			IdNotFound,
			ParamAlreadyDefined,
			GenericParamAlreadyDefined,
			InvalidInitializerListUsage,
			ErrorDeducingInitializerListType,
			ErrorDeducingSwitchConditionType,
			ErrorDeducingArgType,
			ErrorEvaluatingConstSwitchCaseCondition,
			MismatchedSwitchCaseConditionType,
			ErrorDeducingMatchConditionType,
			DuplicatedSwitchCaseBranch,
			ErrorDeducingMatchResultType,
			ErrorEvaluatingConstMatchCaseCondition,
			MismatchedMatchCaseConditionType,
			DuplicatedMatchCaseBranch,
			MissingDefaultMatchCaseBranch,
			InvalidThisUsage,
			NoMatchingFnOverloading,
			UnableToDetermineOverloading,
			ArgsMismatched,
			MissingBindingObject,
			RedundantWithObject,
			LocalVarAlreadyExists,
			InvalidBreakUsage,
			InvalidContinueUsage,
			InvalidCaseLabelUsage,
			TypeIsNotConstructible,
			InvalidCast,
			FunctionOverloadingDuplicated,
			RequiresInitialValue,
			ErrorDeducingExprType,
			ErrorDeducingVarType,
			TypeIsNotUnpackable,
			InvalidVarArgHintDuringInstantiation,
			CannotBeUnpackedInThisContext,
			TypeIsNotSubstitutable,
			RequiresCompTimeExpr,
			TypeArgTypeMismatched,
			InterfaceMethodsConflicted,
			TypeIsNotInitializable,
			MemberIsNotAccessible,
			InvalidEnumBaseType,
			EnumItemIsNotAssignable,
			IncompatibleInitialValueType,
			FunctionOverloadingDuplicatedDuringInstantiation,
			ReturnValueTypeDoesNotMatch,
			DereferencingNull,
			InstanceMemberVarNotInitialized,
			StaticMemberVarNotInitialized,
			ThisNotInitialized,
			ConflictingWithParentMemberDefinitions,
			FnNotOverridable,
			FnShouldBeMarkedAsOverride,
			FnDoesNotOverride,
			LiteralOverflowed,

			ImportLimitExceeded,
			MalformedModuleName,
			ErrorParsingImportedModule,
			ModuleNotFound,
			RegLimitExceeded,

			InvalidMnemonic,

			ErrorWritingCompiledModule
		};

		struct CompilationError {
			ast::TokenRange source_location;
			CompilationErrorKind error_kind;

			CompilationError(CompilationErrorKind error_kind) : error_kind(error_kind) {}
			CompilationError(ast::TokenRange source_location, CompilationErrorKind error_kind) : source_location(source_location), error_kind(error_kind) {}
		};

		SLAKE_FORCEINLINE peff::Option<CompilationError> gen_pinning_io_error_option() {
			return peff::Option<CompilationError>(CompilationError(CompilationErrorKind::PinningIOError));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> gen_out_of_node_index_error_option() {
			return peff::Option<CompilationError>(CompilationError(CompilationErrorKind::OutOfNodeIndex));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> gen_oom_error_option() {
			return peff::Option<CompilationError>(CompilationError(CompilationErrorKind::OutOfMemory));
		}

		struct CompilationEnv {
		private:
			/// @brief Associated global state.
			Global *_global;
			/// @brief Module to be compiled.
			ast::AstNodePin<ast::ModuleNode> _target_module;
			/// @brief Generated compilation errors.
			peff::DynArray<CompilationError> _compilation_errors;

		public:
			SLKC_API CompilationEnv(Global *global) noexcept;

			PEFF_FORCEINLINE peff::Option<CompilationError> push_error(CompilationError &&error) noexcept {
				if (!_compilation_errors.push_back(std::move(error)))
					return gen_oom_error_option();
				return peff::NULLOPT;
			}

			PEFF_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			PEFF_FORCEINLINE std::span<CompilationError> get_errors() const noexcept {
				return _compilation_errors;
			}

			PEFF_FORCEINLINE void set_target_module(const ast::AstNodePin<ast::ModuleNode> &mod) noexcept {
				_target_module = mod;
			}

			PEFF_FORCEINLINE ast::AstNodePin<ast::ModuleNode> get_target_module() const noexcept {
				return _target_module;
			}
		};

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
	}
}

#define SLKC_RETURN_IF_COMP_ERROR(e)                           \
	if (peff::Option<CompilationError> _ = (e); _.has_value()) \
	return std::move(_).value()

#define SLKC_CO_RETURN_IF_COMP_ERROR(e)                        \
	if (peff::Option<CompilationError> _ = (e); _.has_value()) \
	co_return std::move(_).value()

#endif
