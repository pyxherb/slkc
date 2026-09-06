#ifndef _SLKC_AST_NODEDEFS_STMT_H_
#define _SLKC_AST_NODEDEFS_STMT_H_

#include "expr.h"

namespace slkc {
	namespace ast {
		class ExprStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<AstNodePtr<ExprNode>> inner_exprs;

			SLKC_API ExprStmtNode(Global *global);
			SLKC_API ExprStmtNode(const ExprStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ExprStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		struct BindingEntry final {
			/// @brief The binding name.
			GlobalSharedStringRef name;
			/// @brief Type of the binding.
			TypeName type;
			/// @brief Initial value of the binding.
			AstNodePtr<ExprNode> initial_value;

			bool is_var_binding = false;

			TokenIndex sti_name_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_colon = INVALID_TOKEN_INDEX;
			TokenIndex sti_assignment = INVALID_TOKEN_INDEX;

			SLKC_API DumpResult dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			SLKC_API peff::Result<BindingEntry, DuplicationError> duplicate(AstNodeDuplicationContext &context) const noexcept;
		};

		class VarDefStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Binding entries in the let statement.
			peff::DynArray<BindingEntry> bindings;

			TokenIndex sti_let_keyword = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_binding_separators;

			SLKC_API VarDefStmtNode(Global *global);
			SLKC_API VarDefStmtNode(const VarDefStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~VarDefStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ForStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Loop variables for the loop statement.
			/// @note The variables should be implemented as immutable.
			peff::DynArray<BindingEntry> loop_vars;
			/// @brief Condition expression that is evaluated each loop cycle.
			AstNodePtr<ExprNode> condition_expr;
			/// @brief Step expressions that are evaluated at the end of each loop cycle.
			/// @note The step expressions are not evaluated if the user breaks manually.
			peff::DynArray<AstNodePtr<ExprNode>> step_exprs;
			/// @brief Body of the loop statement.
			AstNodePtr<StmtNode> body;

			/// @brief Token index to the for keyword.
			TokenIndex sti_for_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the first semicolon.
			TokenIndex sti_first_semicolon = INVALID_TOKEN_INDEX;
			/// @brief Token index to the second semicolon.
			TokenIndex sti_second_semicolon = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API ForStmtNode(Global *global);
			SLKC_API ForStmtNode(const ForStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ForStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ForEachStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Name of the loop variable.
			GlobalSharedStringRef loop_var_name;
			/// @brief Expression that is evaluated at the beginning of the loop and iterated each cycle.
			AstNodePtr<ExprNode> collection_expr;
			/// @brief Body of the loop statement.
			AstNodePtr<StmtNode> body;

			/// @brief Token index to the for keyword.
			TokenIndex sti_foreach_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the colon.
			TokenIndex sti_colon_index = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API ForEachStmtNode(Global *global);
			SLKC_API ForEachStmtNode(const ForEachStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ForEachStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class WhileStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Condition expression that is evaluated at the beginning of each loop cycle and determines if the loop should continue.
			AstNodePtr<ExprNode> condition_expr;
			/// @brief Body of the loop statement.
			AstNodePtr<StmtNode> body;

			/// @brief Token index to the while keyword.
			TokenIndex sti_while_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API WhileStmtNode(Global *global);
			SLKC_API WhileStmtNode(const WhileStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~WhileStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class DoWhileStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Condition expression that is evaluated at the end of each loop cycle and determines if the loop should continue.
			AstNodePtr<ExprNode> condition_expr;
			/// @brief Body of the loop statement.
			AstNodePtr<StmtNode> body;

			/// @brief Token index to the do keyword.
			TokenIndex sti_do_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the while keyword.
			TokenIndex sti_while_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API DoWhileStmtNode(Global *global);
			SLKC_API DoWhileStmtNode(const DoWhileStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~DoWhileStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class IfStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			AstNodePtr<ExprNode> condition;
			AstNodePtr<StmtNode> true_branch;
			AstNodePtr<StmtNode> false_branch;

			TokenIndex sti_if_keyword = INVALID_TOKEN_INDEX,
					   sti_left_parenthesis = INVALID_TOKEN_INDEX,
					   sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API IfStmtNode(Global *global);
			SLKC_API IfStmtNode(const IfStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~IfStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		struct SwitchStmtBranch final {
			AstNodePtr<ExprNode> pattern;
			AstNodePtr<StmtNode> body;
			TokenIndex sti_case_keyword = INVALID_TOKEN_INDEX,
					   sti_default_keyword = INVALID_TOKEN_INDEX;

			SLAKE_FORCEINLINE bool is_default_branch() const noexcept {
				return !pattern;
			}

			[[nodiscard]] SLKC_API DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			SLKC_API peff::Result<SwitchStmtBranch, DuplicationError> duplicate(AstNodeDuplicationContext &context) const noexcept;
		};

		class SwitchStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			AstNodePtr<ExprNode> condition;
			peff::DynArray<SwitchStmtBranch> branches;

			TokenIndex sti_switch_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;

			SLKC_API SwitchStmtNode(Global *global);
			SLKC_API SwitchStmtNode(const SwitchStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~SwitchStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class BlockStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<AstNodePtr<StmtNode>> inner_stmts;

			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;

			SLKC_API BlockStmtNode(Global *global);
			SLKC_API BlockStmtNode(const BlockStmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~BlockStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
