#ifndef _SLKC_COMP_ENV_H_
#define _SLKC_COMP_ENV_H_

#include <slkc/ast/rgtree.h>
#include <slkc/ast/astnode.h>

namespace slkc {
	namespace comp {
		enum class CompilationErrorKind : int {
			OutOfMemory = 0,
			StackOverflow,
			OutOfRuntimeMemory,
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

			CompilationError(CompilationErrorKind error_kind): error_kind(error_kind) {}
			CompilationError(const ast::GreenNodePtr &node, CompilationErrorKind error_kind): node(node), error_kind(error_kind) {}
		};
	}
}

#endif
