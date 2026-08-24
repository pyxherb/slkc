#ifndef _SLKC_AST_NODEDEFS_FN_H_
#define _SLKC_AST_NODEDEFS_FN_H_

#include "stmt.h"
#include "member.h"

namespace slkc {
	namespace ast {
		class FnNode;

		enum class FnOverloadingKind : uint8_t {
			Regular = 0,
			Coroutine,
			Operator,
		};

		using FnOverloadingFlags = uint8_t;

		constexpr FnOverloadingFlags
			OVERLOADING_FLAG_CONST = 1 << 0,
			OVERLOADING_FLAG_VARARG = 1 << 1,
			OVERLOADING_FLAG_VIRTUAL = 1 << 2,
			OVERLOADING_FLAG_OVERRIDE = 1 << 3;

		class FnOverloadingNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<BindingEntry> params;
			TypeName return_type;
			peff::Option<TypeName> overriden_type;
			AstNodePtr<BlockStmtNode> body;

			FnOverloadingFlags overloading_flags = 0;
			FnOverloadingKind overloading_kind = FnOverloadingKind::Regular;

			TokenIndex sti_fn_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_name = INVALID_TOKEN_INDEX;
			TokenIndex sti_generic_left_angle = INVALID_TOKEN_INDEX;
			TokenIndex sti_generic_right_angle = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_return_type_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_vararg_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_const_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_virtual_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_override_keyword = INVALID_TOKEN_INDEX;

			peff::DynArray<TokenIndex> idx_param_comma_tokens,
				idx_generic_param_comma_tokens;

			SLKC_API FnOverloadingNode(Global *global);
			SLKC_API FnOverloadingNode(const FnOverloadingNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~FnOverloadingNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class FnNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<AstNodePtr<FnOverloadingNode>> overloadings;

			SLKC_API FnNode(Global *global);
			SLKC_API FnNode(const FnNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~FnNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
