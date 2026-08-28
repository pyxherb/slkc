#ifndef _SLKC_COMP_ENV_H_
#define _SLKC_COMP_ENV_H_

#include <slkc/ast/rgtree.h>
#include <slkc/ast/utils.h>

namespace slkc {
	namespace comp {
		enum class CompilationErrorKind : int {
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

			ImportLimitExceeded,
			MalformedModuleName,
			ErrorParsingImportedModule,
			ModuleNotFound,
			RegLimitExceeded,

			InvalidMnemonic,

			ErrorWritingCompiledModule
		};

		struct CompilationError {
			ast::GreenNodePtr node;
			CompilationErrorKind error_kind;

			CompilationError(CompilationErrorKind error_kind) : error_kind(error_kind) {}
			CompilationError(const ast::GreenNodePtr &node, CompilationErrorKind error_kind) : node(node), error_kind(error_kind) {}
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
	}
}

#define SLKC_RETURN_IF_COMP_ERROR(e)                           \
	if (peff::Option<CompilationError> _ = (e); _.has_value()) \
	return std::move(_).value()

#define SLKC_CO_RETURN_IF_COMP_ERROR(e)                        \
	if (peff::Option<CompilationError> _ = (e); _.has_value()) \
	co_return std::move(_).value()

#endif
