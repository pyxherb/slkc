#ifndef _SLKC_AST_NODEDEFS_EXPR_H_
#define _SLKC_AST_NODEDEFS_EXPR_H_

#include "idref.h"

namespace slkc {
	namespace ast {
		enum class ExprKind {
			Unary,	  // Unary operation
			Binary,	  // Binary operation
			Ternary,  // Ternary operation
			IdRef,	  // Identifier reference

			HeadedIdRef,  // Headed identifier reference

			I8,		 // i8 literal
			I16,	 // i16 literal
			I32,	 // i32 literal
			I64,	 // i64 literal
			U8,		 // u8 literal
			U16,	 // u16 literal
			U32,	 // u32 literal
			U64,	 // u64 literal
			F32,	 // f32 literal
			F64,	 // f64 literal
			String,	 // String literal
			Bool,	 // bool literal
			Null,	 // null

			InitializerList,  // Initializer list

			Call,  // Call

			New,  // New

			Alloca,	 // Alloca

			Cast,  // Cast

			Match,	// Match expression

			Group,	// Expression group
		};

		class ExprNode : public Node {
		protected:
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		private:
			const ExprKind _expr_kind;
			bool _is_bad;

		public:
			SLKC_API ExprNode(ExprKind expr_kind, Global *global);
			SLKC_API ExprNode(const ExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~ExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();

			SLAKE_FORCEINLINE ExprKind get_expr_kind() const noexcept {
				return _expr_kind;
			}

			SLAKE_FORCEINLINE bool is_bad() const noexcept {
				return _is_bad;
			}

			SLAKE_FORCEINLINE void set_bad(bool bad) noexcept {
				_is_bad = bad;
			}
		};

		enum class UnaryOp : uint8_t {
			LNot,	   // Logical NOT !
			Not,	   // Bitwise NOT ~
			Neg,	   // Negation -
			Move,	   // Move +
			Unpacking  // Unpacking ...
		};

		class UnaryExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> operand;
			UnaryOp unary_op;
			TokenIndex sti_operator = INVALID_TOKEN_INDEX;

			SLKC_API UnaryExprNode(Global *global);
			SLKC_API UnaryExprNode(const UnaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~UnaryExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		enum class BinaryOp : uint8_t {
			Add = 0,  // Adding +
			Sub,	  // Subtraction -
			Mul,	  // Multiplicaton *
			Div,	  // Division /
			Mod,	  // Modulo %
			And,	  // Bitwise AND &
			Or,		  // Bitwise OR |
			Xor,	  // Bitwise XOR ^
			LAnd,	  // Logical AND &&
			LOr,	  // Logical OR ||
			Shl,	  // Left-shift <<
			Shr,	  // Right-shift >>

			Assign,		// Assignment =
			AddAssign,	// Adding then asignment +=
			SubAssign,	// Subtraction then assignment -=
			MulAssign,	// Multiplication then assignment *=
			DivAssign,	// Divison then assignment /=
			ModAssign,	// Modulo then assignment %=
			AndAssign,	// Bitwise AND then assignment &=
			OrAssign,	// Bitwise OR then assignment |=
			XorAssign,	// Bitwise XOR then assignment ^=
			ShlAssign,	// Left-shift then assignment <<=
			ShrAssign,	// Right-shift then assignment >>=

			Eq,			// Equality ==
			Neq,		// Inequality !=
			PhyEq,		// Physical Equality ===
			PhyNeq,		// Physical Inequality !==
			Lt,			// Less than <
			Gt,			// Greater than >
			LtEq,		// Less than or equal <=
			GtEq,		// Greater than or equal >=
			Cmp,		// Three-way comparison <=>
			Subscript,	// Subscript []

			Comma,	// Comma ,
		};

		class BinaryExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> lhs, rhs;
			BinaryOp binary_op;

			TokenIndex sti_operator_prefix = INVALID_TOKEN_INDEX;
			TokenIndex sti_operator_infix = INVALID_TOKEN_INDEX;
			TokenIndex sti_operator_suffix = INVALID_TOKEN_INDEX;

			SLKC_API BinaryExprNode(Global *global);
			SLKC_API BinaryExprNode(const BinaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~BinaryExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class TernaryExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> condition, true_branch, false_branch;

			TokenIndex sti_operator_question = INVALID_TOKEN_INDEX;
			TokenIndex sti_operator_colon = INVALID_TOKEN_INDEX;

			SLKC_API TernaryExprNode(Global *global);
			SLKC_API TernaryExprNode(const TernaryExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~TernaryExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class IdRefExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			OwnedIdRef id_ref;

			SLKC_API IdRefExprNode(Global *global);
			SLKC_API IdRefExprNode(const IdRefExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~IdRefExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class HeadedIdRefExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> head_expr;
			OwnedIdRef id_ref;

			TokenIndex sti_separator = INVALID_TOKEN_INDEX;

			SLKC_API HeadedIdRefExprNode(Global *global);
			SLKC_API HeadedIdRefExprNode(const HeadedIdRefExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~HeadedIdRefExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I8LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			int8_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API I8LiteralExprNode(Global *global, int8_t literal);
			SLKC_API I8LiteralExprNode(const I8LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~I8LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I16LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			int16_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API I16LiteralExprNode(Global *global, int16_t literal);
			SLKC_API I16LiteralExprNode(const I16LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~I16LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I32LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			int32_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API I32LiteralExprNode(Global *global, int32_t literal);
			SLKC_API I32LiteralExprNode(const I32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~I32LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I64LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			int64_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API I64LiteralExprNode(Global *global, int64_t literal);
			SLKC_API I64LiteralExprNode(const I64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~I64LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U8LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			uint8_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API U8LiteralExprNode(Global *global, uint8_t literal);
			SLKC_API U8LiteralExprNode(const U8LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~U8LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U16LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			uint16_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API U16LiteralExprNode(Global *global, uint16_t literal);
			SLKC_API U16LiteralExprNode(const U16LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~U16LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U32LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			uint32_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API U32LiteralExprNode(Global *global, uint32_t literal);
			SLKC_API U32LiteralExprNode(const U32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~U32LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U64LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			uint64_t literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API U64LiteralExprNode(Global *global, uint64_t literal);
			SLKC_API U64LiteralExprNode(const U64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~U64LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class F32LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			float literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API F32LiteralExprNode(Global *global, float literal);
			SLKC_API F32LiteralExprNode(const F32LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~F32LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class F64LiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			double literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API F64LiteralExprNode(Global *global, double literal);
			SLKC_API F64LiteralExprNode(const F64LiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~F64LiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class StringLiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			GlobalSharedStringRef literal;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API StringLiteralExprNode(Global *global, GlobalSharedStringRef literal);
			SLKC_API StringLiteralExprNode(const StringLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~StringLiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class BoolLiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			bool literal = 0;

			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API BoolLiteralExprNode(Global *global, bool literal);
			SLKC_API BoolLiteralExprNode(const BoolLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~BoolLiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class NullLiteralExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_literal = INVALID_TOKEN_INDEX;

			SLKC_API NullLiteralExprNode(Global *global);
			SLKC_API NullLiteralExprNode(const NullLiteralExprNode &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~NullLiteralExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class InitializerListExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::DynArray<NodePtr<ExprNode>> elements;

			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_element_separators;

			SLKC_API InitializerListExprNode(Global *global);
			SLKC_API InitializerListExprNode(const InitializerListExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~InitializerListExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class CallExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> target;
			peff::DynArray<NodePtr<ExprNode>> args;

			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_arg_separators;

			SLKC_API CallExprNode(Global *global);
			SLKC_API CallExprNode(const CallExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~CallExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class NewExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TypeName target_type;
			peff::DynArray<NodePtr<ExprNode>> args;

			TokenIndex sti_new_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_arg_separators;

			SLKC_API NewExprNode(Global *global);
			SLKC_API NewExprNode(const NewExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~NewExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class AllocaExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TypeName target_type;
			peff::DynArray<NodePtr<ExprNode>> args;

			TokenIndex sti_alloca_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_arg_separators;

			SLKC_API AllocaExprNode(Global *global);
			SLKC_API AllocaExprNode(const AllocaExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~AllocaExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class CastExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TypeName target_type;
			NodePtr<ExprNode> operand;
			bool is_nullable = false;

			TokenIndex sti_as_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_nullable_token = INVALID_TOKEN_INDEX;

			SLKC_API CastExprNode(Global *global);
			SLKC_API CastExprNode(const CastExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~CastExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		struct MatchExprBranch final {
			NodePtr<ExprNode> pattern;
			NodePtr<ExprNode> result_value;
			TokenIndex sti_case_keyword = INVALID_TOKEN_INDEX,
					   sti_default_keyword = INVALID_TOKEN_INDEX,
					   sti_colon = INVALID_TOKEN_INDEX;

			SLAKE_FORCEINLINE bool is_default_branch() const noexcept {
				return !pattern;
			}

			[[nodiscard]] SLKC_API peff::Result<MatchExprBranch, DuplicationError> do_duplicate(DuplicationContext &context) const noexcept;
			[[nodiscard]] SLKC_API DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;
		};

		class MatchExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> condition;
			TypeName return_type;
			peff::DynArray<MatchExprBranch> branches;

			TokenIndex sti_match_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_return_type_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_left_brace = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_brace = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_case_separators;

			SLKC_API MatchExprNode(Global *global);
			SLKC_API MatchExprNode(const MatchExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~MatchExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class GroupExprNode final : public ExprNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			NodePtr<ExprNode> operand;

			TokenIndex sti_left_parenthesis = INVALID_TOKEN_INDEX;
			TokenIndex sti_right_parenthesis = INVALID_TOKEN_INDEX;

			SLKC_API GroupExprNode(Global *global);
			SLKC_API GroupExprNode(const GroupExprNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~GroupExprNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
