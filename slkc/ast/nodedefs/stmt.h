#ifndef _SLKC_AST_NODEDEFS_STMT_H_
#define _SLKC_AST_NODEDEFS_STMT_H_

#include "expr.h"

namespace slkc {
	namespace ast {
		enum class StmtKind : uint8_t {
			Expr = 0,  // Expression
			Let,	   // Let binding
			Break,	   // Break
			Continue,  // Continue
			For,	   // For
			ForEach,   // For each
			While,	   // While
			DoWhile,   // Do while
			Return,	   // Return
			Yield,	   // Yield
			If,		   // If
			Switch,	   // Switch
			Block,	   // Code block
		};

		class StmtNode : public Node {
		protected:
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		private:
			const StmtKind _stmt_kind;
			bool _is_bad;

		public:
			SLKC_API StmtNode(StmtKind stmt_kind, Global *global);
			SLKC_API StmtNode(const StmtNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~StmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();

			PEFF_FORCEINLINE StmtKind get_stmt_kind() const noexcept {
				return _stmt_kind;
			}

			PEFF_FORCEINLINE bool is_bad() const noexcept {
				return _is_bad;
			}

			PEFF_FORCEINLINE void set_bad(bool bad) noexcept {
				_is_bad = bad;
			}
		};

		class ExprStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> inner_expr;

			SLKC_API ExprStmtNode(Global *global);
			SLKC_API ExprStmtNode(const ExprStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ExprStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		struct BindingEntry final {
			/// @brief The binding name.
			GlobalSharedStringRef name;
			/// @brief Type of the binding.
			TypeName type;
			/// @brief Initial value of the binding.
			NodePtr<ExprNode> initial_value;

			TokenIndex sti_name_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_colon = INVALID_TOKEN_INDEX;
			TokenIndex sti_assignment = INVALID_TOKEN_INDEX;

			SLKC_API DumpResult dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			SLKC_API peff::Result<BindingEntry, DuplicationError> duplicate(DuplicationContext &context) const noexcept;
		};

		enum class VarDefBindingType : uint8_t {
			Var = 0,
			Let
		};

		class VarDefStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Binding entries in the let statement.
			peff::DynArray<BindingEntry> bindings;
			
			VarDefBindingType binding_type;

			TokenIndex sti_let_keyword = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_binding_separators;

			SLKC_API VarDefStmtNode(Global *global);
			SLKC_API VarDefStmtNode(const VarDefStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~VarDefStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class BreakStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Token index to the break keyword.
			TokenIndex sti_break_keyword = INVALID_TOKEN_INDEX,
					   sti_semicolon = INVALID_TOKEN_INDEX;

			SLKC_API BreakStmtNode(Global *global);
			SLKC_API BreakStmtNode(const BreakStmtNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~BreakStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ContinueStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Values for the next loop cycle, for the for loop.
			/// @note This array should be left empty if the continue statement is not in a for loop.
			peff::DynArray<NodePtr<ExprNode>> continue_values;

			/// @brief Token index to the continue keyword.
			TokenIndex sti_continue_keyword = INVALID_TOKEN_INDEX,
					   sti_semicolon = INVALID_TOKEN_INDEX;
			/// @brief Token indices of the continue values separators (,).
			peff::DynArray<TokenIndex> sti_continue_values_separators;

			SLKC_API ContinueStmtNode(Global *global);
			SLKC_API ContinueStmtNode(const ContinueStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ContinueStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ForStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Loop variables for the loop statement.
			/// @note The variables should be implemented as immutable.
			peff::DynArray<BindingEntry> loop_vars;
			/// @brief Condition expression that is evaluated each loop cycle.
			NodePtr<ExprNode> condition_expr;
			/// @brief Step expression that is evaluated at the end of each loop cycle.
			/// @note The step is not evaluated if the user breaks manually.
			NodePtr<ExprNode> step_expr;
			/// @brief Body of the loop statement.
			NodePtr<StmtNode> body;

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
			SLKC_API ForStmtNode(const ForStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ForStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ForEachStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Name of the loop variable.
			GlobalSharedStringRef loop_var_name;
			/// @brief Expression that is evaluated at the beginning of the loop and iterated each cycle.
			NodePtr<ExprNode> collection_expr;
			/// @brief Body of the loop statement.
			NodePtr<StmtNode> body;

			/// @brief Token index to the for keyword.
			TokenIndex sti_foreach_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the colon.
			TokenIndex sti_colon_index = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API ForEachStmtNode(Global *global);
			SLKC_API ForEachStmtNode(const ForEachStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ForEachStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class WhileStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Condition expression that is evaluated at the beginning of each loop cycle and determines if the loop should continue.
			NodePtr<ExprNode> condition_expr;
			/// @brief Body of the loop statement.
			NodePtr<StmtNode> body;

			/// @brief Token index to the while keyword.
			TokenIndex sti_while_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API WhileStmtNode(Global *global);
			SLKC_API WhileStmtNode(const WhileStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~WhileStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class DoWhileStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			/// @brief Condition expression that is evaluated at the end of each loop cycle and determines if the loop should continue.
			NodePtr<ExprNode> condition_expr;
			/// @brief Body of the loop statement.
			NodePtr<StmtNode> body;

			/// @brief Token index to the do keyword.
			TokenIndex sti_do_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the while keyword.
			TokenIndex sti_while_keyword = INVALID_TOKEN_INDEX;
			/// @brief Token index to the left parenthesis.
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			/// @brief Token index to the right parenthesis.
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API DoWhileStmtNode(Global *global);
			SLKC_API DoWhileStmtNode(const DoWhileStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~DoWhileStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ReturnStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> return_value;

			TokenIndex sti_return_keyword = INVALID_TOKEN_INDEX,
					   sti_semicolon = INVALID_TOKEN_INDEX;

			SLKC_API ReturnStmtNode(Global *global);
			SLKC_API ReturnStmtNode(const ReturnStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ReturnStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class YieldStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> return_value;

			TokenIndex sti_yield_keyword = INVALID_TOKEN_INDEX,
					   sti_semicolon = INVALID_TOKEN_INDEX;

			SLKC_API YieldStmtNode(Global *global);
			SLKC_API YieldStmtNode(const YieldStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~YieldStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class IfStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> condition;
			NodePtr<StmtNode> true_branch;
			NodePtr<StmtNode> false_branch;

			TokenIndex sti_if_keyword = INVALID_TOKEN_INDEX,
					   sti_left_parenthesis = INVALID_TOKEN_INDEX,
					   sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API IfStmtNode(Global *global);
			SLKC_API IfStmtNode(const IfStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~IfStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		struct SwitchStmtBranch final {
			NodePtr<ExprNode> pattern;
			NodePtr<StmtNode> body;
			TokenIndex sti_case_keyword = INVALID_TOKEN_INDEX,
					   sti_default_keyword = INVALID_TOKEN_INDEX;

			PEFF_FORCEINLINE bool is_default_branch() const noexcept {
				return !pattern;
			}

			[[nodiscard]] SLKC_API DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			SLKC_API peff::Result<SwitchStmtBranch, DuplicationError> duplicate(DuplicationContext &context) const noexcept;
		};

		class SwitchStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> condition;
			peff::DynArray<MatchExprBranch> branches;

			TokenIndex sti_switch_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;

			SLKC_API SwitchStmtNode(Global *global);
			SLKC_API SwitchStmtNode(const SwitchStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~SwitchStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class BlockStmtNode final : public StmtNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<NodePtr<StmtNode>> inner_stmts;

			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;

			SLKC_API BlockStmtNode(Global *global);
			SLKC_API BlockStmtNode(const BlockStmtNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~BlockStmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
