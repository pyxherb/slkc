#ifndef _SLKC_COMP_BASEDEFS_H_
#define _SLKC_COMP_BASEDEFS_H_

#include <slkc/basedefs.h>
#include <slkc/ast/basedefs.h>
#include <wandjson/dump.h>
#include <cstdint>
#include <limits>

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
	}
}

#endif
