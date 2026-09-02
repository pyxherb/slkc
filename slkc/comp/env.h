#ifndef _SLKC_COMP_ENV_H_
#define _SLKC_COMP_ENV_H_

#include <slkc/ast/rgtree.h>
#include <slkc/ast/utils.h>
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
			ast::Global *_global;
			/// @brief Module to be compiled.
			ast::AstNodePin<ast::ModuleNode> _target_module;
			/// @brief Generated compilation errors.
			peff::DynArray<CompilationError> _compilation_errors;

		public:
			SLKC_API CompilationEnv(ast::Global *global, const ast::AstNodePin<ast::ModuleNode> &target_module) noexcept;

			PEFF_FORCEINLINE peff::Option<CompilationError> push_error(CompilationError &&error) noexcept {
				if (!_compilation_errors.push_back(std::move(error)))
					return gen_oom_error_option();
				return peff::NULLOPT;
			}

			PEFF_FORCEINLINE ast::Global *get_global() const noexcept {
				return _global;
			}

			PEFF_FORCEINLINE std::span<CompilationError> get_errors() const noexcept {
				return _compilation_errors;
			}

			PEFF_FORCEINLINE ast::AstNodePin<ast::ModuleNode> get_target_module() const noexcept {
				return _target_module;
			}
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
