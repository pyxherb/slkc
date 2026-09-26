#ifndef _SLKC_AST_ERROR_H_
#define _SLKC_AST_ERROR_H_

#include "astnode.h"

namespace slkc {
	namespace ast {
		enum class CompilationErrorKind : int {
			/// @brief The compiler has encountered an OOM error.
			OutOfMemory = 0,

			/// @brief The compiler has encountered a stack overflow error.
			StackOverflow,
			/// @brief Target operand of the calling expression is not callable.
			TargetIsNotCallable,
			/// @brief Target operand of the unpacking expression is not unpackable.
			TargetIsNotUnpackable,
			/// @brief No matched function overloading for given arguments.
			NoSuchFnOverloading,
			/// @brief The function invocation with given arguments is ambiguous.
			AmbiguousFnCall,
			/// @brief Incompatible types of operands in binary expression.
			IncompatibleOperand,
			/// @brief No such operand combination for given binary operator.
			OperatorNotFound,
			/// @brief The operator invocation with given arguments is ambiguous.
			AmbiguousOperatorCall,
			/// @brief Generic argument number for the type is mismatched.
			MismatchedGenericArgNumber,
			/// @brief Expecting a type name in such a place.
			ExpectingTypeName,
			/// @brief Expecting a class name in such a place.
			ExpectingClassName,
			/// @brief Expecting a interface name in such a place.
			ExpectingInterfaceName,
			/// @brief There is one or more abstract methods implemented in the type definition.
			AbstractMethodNotImplemented,
			/// @brief Cyclic inheritance in classes is detected.
			CyclicInheritedClass,
			/// @brief Cyclic inheritance in interfaces is detected.
			CyclicInheritedInterface,
			/// @brief (Co-) recursed type is detected.
			RecursedType,
			/// @brief Expecting an ID in such a place.
			ExpectingId,
			/// @brief Entity referred by specified ID reference was not found.
			IdNotFound,
			/// @brief Parameter with the name is already defined.
			ParamAlreadyDefined,
			/// @brief Generic parameter with the name is already defined.
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

			ErrorWritingCompiledModule
		};

		struct CompilationError {
			TokenRange token_range;
		};
	}
}

#endif
