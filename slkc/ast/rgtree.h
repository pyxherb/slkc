#ifndef _SLKC_AST_RGTREE_H_
#define _SLKC_AST_RGTREE_H_

#include "global.h"
#include "parser/lexer.h"
#include <peff/advutils/shared_ptr.h>
#include <peff/containers/btree_map.h>

namespace slkc {
	namespace ast {
		namespace GreenNodeKind {
			enum {
				/// @brief Invalid node type.
				Invalid = 0x10000000,

				/// @brief An operator name.
				OperatorName,

				/// @brief A type name.
				TypeName,

				/// @brief A generic argument in the identifier entry.
				GenericArg,
				/// @brief An identifier name.
				IdName,
				/// @brief An identifier reference entry.
				IdRefEntry,
				/// @brief An identifier reference.
				IdRef,

				/// @brief An expression.
				Expr,

				/// @brief A match case.
				MatchCase,

				/// @brief A variable bindings.
				VarBinding,

				/// @brief Variable bindings.
				VarBindings,

				/// @brief Arguments passing to a function.
				Args,

				/// @brief A generic constraint applied to a generic parameter.
				GenericConstraint,
				/// @brief A generic parameter applied to a member.
				GenericParam,

				/// @brief A parameter in a function.
				Param,

				/// @brief A function declaration.
				FnDecl,
				/// @brief A function definition.
				FnDef,
				/// @brief An operator declaration.
				OperatorDecl,
				/// @brief An operator definition.
				OperatorDef,

				/// @brief A statement.
				Stmt,

				/// @brief A case of a switch statement.
				SwitchCase,
				/// @brief Default case of a switch statement.
				SwitchDefaultCase,

				/// @brief Top-level module definition.
				Module,

				/// @brief Inheritance slot on a class, generic constraint, etc.
				InheritanceSlot,

				/// @brief An implementation item in an implementation list.
				ImplItem,
				/// @brief An implementation list on a class, interface, generic constraint, etc
				ImplList,

				/// @brief A class definition.
				ClassDef,

				/// @brief An interface definition.
				InterfaceDef,

				/// @brief A trait definition.
				TraitDef,

				/// @brief An exception definition.
				ExceptDef,

				/// @brief A struct definition.
				StructDef,

				/// @brief An unknown kind of enumeration declaration.
				UnknownEnumDecl,

				/// @brief A constant enumeration definition.
				ConstEnumDef,

				/// @brief A scoped enumeration definition.
				ScopedEnumDef,

				/// @brief A constant and scoped enumeration item.
				ConstAndScopedEnumItem,

				/// @brief A union enumeartion definition.
				UnionEnumDef,

				/// @brief A union enumeration case.
				UnionEnumCase,

				/// @brief An attribute definition.
				AttributeDef,

				/// @brief An import item.
				ImportItem,

				/// @brief A global variable declaration.
				GlobalVar,

				Max
			};
		}

		enum class GreenNodeTypeNameKind : uint8_t {
			Invalid = 0,
			/// @brief An i8 type name.
			I8TypeName,
			/// @brief An i16 type name.
			I16TypeName,
			/// @brief An i32 type name.
			I32TypeName,
			/// @brief An i64 type name.
			I64TypeName,
			/// @brief An isize type name.
			ISizeTypeName,
			/// @brief A u8 type name.
			U8TypeName,
			/// @brief A u16 type name.
			U16TypeName,
			/// @brief A u32 type name.
			U32TypeName,
			/// @brief A u64 type name.
			U64TypeName,
			/// @brief A usize type name.
			USizeTypeName,
			/// @brief An f32 type name.
			F32TypeName,
			/// @brief An f64 type name.
			F64TypeName,
			/// @brief A string type name.
			StringTypeName,
			/// @brief A bool type name.
			BoolTypeName,
			/// @brief A void type name.
			VoidTypeName,
			/// @brief An object type name.
			ObjectTypeName,
			/// @brief A any type name.
			AnyTypeName,
			/// @brief A never type name.
			NeverTypeName,
			/// @brief A custom type name.
			CustomTypeName,
			/// @brief An array type name.
			ArrayTypeName,
		};

		enum class GreenNodeExprKind : uint8_t {
			Invalid = 0,

			/// @brief A unary expression.
			Unary,

			/// @brief A binary expression.
			Binary,

			/// @brief A ternary expression.
			Ternary,

			/// @brief An identifier reference expression.
			IdRef,

			/// @brief An identifier reference expression with a preceding expression.
			HeadedIdRef,

			/// @brief An i8-typed integer literal expression.
			I8Literal,

			/// @brief An i16-typed integer literal expression.
			I16Literal,

			/// @brief An i32-typed integer literal expression.
			I32Literal,

			/// @brief An i64-typed integer literal expression.
			I64Literal,

			/// @brief An u8-typed integer literal expression.
			U8Literal,

			/// @brief An u16-typed integer literal expression.
			U16Literal,

			/// @brief An u32-typed integer literal expression.
			U32Literal,

			/// @brief An u64-typed integer literal expression.
			U64Literal,

			/// @brief An f32-typed floating-point literal expression.
			F32Literal,

			/// @brief An f64-typed floating-point literal expression.
			F64Literal,

			/// @brief A string literal expression.
			StringLiteral,

			/// @brief A boolean expression.
			BoolLiteral,

			/// @brief A null expression.
			NullLiteral,

			/// @brief An initializer list expression.
			InitializerList,

			/// @brief A function call expression.
			Call,

			/// @brief A index-based subscript expression.
			Subscript,

			/// @brief A new operation expression.
			New,

			/// @brief A type-casting expression.
			Cast,

			/// @brief A match expression.
			Match,

			/// @brief A grouping (parenthesized) expression.
			Group,
		};

		enum class GreenNodeUnaryExprOp : uint8_t {
			LNot,	   // Logical NOT !
			Not,	   // Bitwise NOT ~
			Neg,	   // Negation -
			Move,	   // Move +
			Unpacking  // Unpacking ...
		};

		enum class GreenNodeBinaryExprOp : uint8_t {
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

			Eq,		 // Equality ==
			Neq,	 // Inequality !=
			PhyEq,	 // Physical Equality ===
			PhyNeq,	 // Physical Inequality !==
			Lt,		 // Less than <
			Gt,		 // Greater than >
			LtEq,	 // Less than or equal <=
			GtEq,	 // Greater than or equal >=
			Cmp,	 // Three-way comparison <=>

			Comma,	// Comma ,
		};

		enum class GreenNodeStmtKind : uint8_t {
			Invalid = 0,

			/// @brief An if statement.
			IfStmt,

			/// @brief A for statement.
			ForStmt,

			/// @brief A while statement.
			WhileStmt,

			/// @brief A do-while statement.
			DoWhileStmt,

			/// @brief A local variable statement.
			LocalVarStmt,

			/// @brief A break statement.
			BreakStmt,

			/// @brief A continue statement.
			ContinueStmt,

			/// @brief A return statement.
			ReturnStmt,

			/// @brief A yield statement.
			YieldStmt,

			/// @brief A block statement.
			BlockStmt,

			/// @brief A switch statement.
			SwitchStmt,
		};

		SLAKE_FORCEINLINE bool is_token_node_kind(TokenKind kind) noexcept {
			return kind >= static_cast<uint32_t>(TokenId::End) && kind < static_cast<uint32_t>(TokenId::MaxToken);
		}

		struct GreenNode;

		class GreenNodePin final {
		private:
			using ThisType = GreenNodePin;
			Global *_global;
			GreenNodeIndex _green_node_index;
			union {
				GreenNode *_ptr;
				PinFailReason _fail_reason;
			};

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, AstNodeIndex node_index) {
				_global = global;
				_green_node_index = node_index;
				global->pin_green_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_global && (_green_node_index != INVALID_GREEN_NODE_INDEX))
					_global->unpin_green_node(_green_node_index);
			}

			SLAKE_FORCEINLINE GreenNodePin() : _global(nullptr), _green_node_index(INVALID_GREEN_NODE_INDEX), _ptr(nullptr) {}
			SLAKE_FORCEINLINE explicit GreenNodePin(Global *global, AstNodeIndex node_index, GreenNode *ptr) : _global(global), _green_node_index(node_index), _ptr(ptr) {
			}
			SLAKE_FORCEINLINE explicit GreenNodePin(PinFailReason reason) : _global(nullptr), _green_node_index(INVALID_GREEN_NODE_INDEX), _fail_reason(reason) {
			}
			SLAKE_FORCEINLINE ~GreenNodePin() {
				reset();
			}
			SLAKE_FORCEINLINE GreenNodePin(const ThisType &rhs) noexcept : _global(rhs._global), _green_node_index(rhs._green_node_index), _ptr(rhs._ptr) {
				_global->pin_green_node(_green_node_index);
			}
			SLAKE_FORCEINLINE GreenNodePin(ThisType &&rhs) noexcept : _global(rhs._global), _green_node_index(rhs._green_node_index), _ptr(rhs._ptr) {
				rhs._global = nullptr;
				rhs._green_node_index = INVALID_AST_NODE_INDEX;
				rhs._ptr = nullptr;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._green_node_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_green_node_index = rhs._green_node_index;
				_ptr = rhs._ptr;
				rhs._global = nullptr;
				rhs._green_node_index = INVALID_AST_NODE_INDEX;
				rhs._ptr = nullptr;

				return *this;
			}

			SLAKE_FORCEINLINE GreenNode *get() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE AstNodeIndex get_index() const noexcept {
				return _green_node_index;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE GreenNode *operator->() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_green_node_index > rhs._green_node_index)
					return 1;
				if (_green_node_index < rhs._green_node_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _green_node_index < rhs._green_node_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _green_node_index > rhs._green_node_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _green_node_index == rhs._green_node_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _green_node_index != rhs._green_node_index;
			}

			SLAKE_FORCEINLINE operator bool() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE bool is_fail() const noexcept {
				return !_global;
			}

			SLAKE_FORCEINLINE PinFailReason get_fail_reason() const noexcept {
				return _fail_reason;
			}
		};

		class GreenNodePtr final {
		private:
			using ThisType = GreenNodePtr;
			Global *_global;
			AstNodeIndex _node_index;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, AstNodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->ref_green_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_node_index != INVALID_AST_NODE_INDEX)
					_global->unref_green_node(_node_index);
			}

			SLAKE_FORCEINLINE GreenNodePtr() : _global(nullptr), _node_index(INVALID_AST_NODE_INDEX) {}
			SLAKE_FORCEINLINE GreenNodePtr(const GreenNodePin &pin) : _global(pin.get_global()), _node_index(pin.get_index()) {
				_global->ref_green_node(_node_index);
			}
			SLAKE_FORCEINLINE explicit GreenNodePtr(Global *global, GreenNodeIndex node_index) : _global(global), _node_index(node_index) {
				global->ref_green_node(node_index);
			}
			SLAKE_FORCEINLINE ~GreenNodePtr() {
				reset();
			}

			SLAKE_FORCEINLINE GreenNodePtr(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->ref_green_node(_node_index);
			}
			SLAKE_FORCEINLINE GreenNodePtr(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_AST_NODE_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._node_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_node_index = rhs._node_index;
				rhs._global = nullptr;
				rhs._node_index = INVALID_AST_NODE_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE AstNodeIndex get_index() const noexcept {
				return _node_index;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			SLAKE_FORCEINLINE GreenNodePin pin() const noexcept {
				auto result = _global->pin_green_node(_node_index);
				if (result.has_error())
					return GreenNodePin(std::move(result).error());
				return GreenNodePin(_global, _node_index, std::move(result).value());
			}

			SLAKE_FORCEINLINE static GreenNodePtr from_pin(GreenNodePin pin) noexcept {
				return GreenNodePtr(pin.get_global(), pin.get_index());
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_node_index > rhs._node_index)
					return 1;
				if (_node_index < rhs._node_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index < rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index > rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index == rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index != rhs._node_index;
			}

			SLAKE_FORCEINLINE operator bool() const noexcept {
				return _node_index != INVALID_AST_NODE_INDEX;
			}
		};

		using GreenNodeChildList = peff::DynArray<std::variant<GreenNodePtr, TokenPtr>>;

		struct TypeNameGreenNodeExData {
			GreenNodeTypeNameKind type_name_kind;
		};

		struct ExprGreenNodeExData {
			GreenNodeExprKind expr_kind;
			union {
				GreenNodeUnaryExprOp unary_expr_op;
				GreenNodeBinaryExprOp binary_expr_op;
			};

			SLAKE_FORCEINLINE ExprGreenNodeExData(GreenNodeExprKind kind) : expr_kind(kind) {}
			SLAKE_FORCEINLINE ExprGreenNodeExData(GreenNodeUnaryExprOp op) : expr_kind(GreenNodeExprKind::Unary), unary_expr_op(op) {}
			SLAKE_FORCEINLINE ExprGreenNodeExData(GreenNodeBinaryExprOp op) : expr_kind(GreenNodeExprKind::Binary), binary_expr_op(op) {}
		};

		struct StmtGreenNodeExData {
			GreenNodeStmtKind stmt_kind;
		};

		enum class GreenNodeOperationResult : uint8_t {
			Success = 0,
			PinIOError,
			OutOfMemory,
			OutOfNodeIndex,
		};

		struct GreenNode final {
		private:
			GreenNode *_next_destructible = nullptr;
			Global *_global;
			GreenNodeIndex _node_index;

			friend class Global;

		public:
			GreenNodeChildList children;
			TextWidth text_width = 0;
			std::variant<std::monostate, TypeNameGreenNodeExData, ExprGreenNodeExData, StmtGreenNodeExData> exdata;
			TokenKind node_kind;

			SLKC_API GreenNode(Global *global);
			SLKC_API ~GreenNode();

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE AstNodeIndex get_node_index() const noexcept {
				return _node_index;
			}

			SLAKE_FORCEINLINE void set_node_index(AstNodeIndex index) noexcept {
				_node_index = index;
			}

			SLAKE_FORCEINLINE bool push_child(GreenNodePtr child) noexcept {
				return children.push_back(std::move(child));
			}

			SLKC_API GreenNodeOperationResult compute_text_width_shallow() noexcept;
		};

		SLKC_API GreenNodeOperationResult compute_green_node_text_width_deep(GreenNodePin root, peff::Alloc *allocator, bool forced_update = false) noexcept;

		struct GreenNodeChildrenIndex {
		private:
			peff::HashMap<TokenKind, size_t> children_index;

		public:
			SLAKE_FORCEINLINE GreenNodeChildrenIndex(peff::Alloc *allocator) : children_index(allocator) {
			}
			GreenNodeChildrenIndex(GreenNodeChildrenIndex &&) = default;

			GreenNodeChildrenIndex &operator=(GreenNodeChildrenIndex &&) = default;

			SLKC_API ~GreenNodeChildrenIndex();

			SLKC_API GreenNodeOperationResult index_node(const GreenNodePin &node) noexcept;
			SLAKE_FORCEINLINE void reset() noexcept {
				children_index.clear();
			}
		};

		SLAKE_FORCEINLINE GreenNodePin make_green_node(Global *global) {
			GreenNode *node = peff::alloc_and_construct<GreenNode>(global->get_allocator(), alignof(GreenNode), global);
			if (!node)
				return GreenNodePin();
			peff::ScopeGuard sg([global, node]() noexcept {
				peff::destroy_and_release<GreenNode>(global->get_allocator(), node, alignof(GreenNode));
			});

			{
				auto result = global->map_green_node(node);
				if (!result.has_value())
					return GreenNodePin(PinFailReason::OutOfMemory);
				if (result.value() == INVALID_AST_NODE_INDEX)
					return GreenNodePin(PinFailReason::OutOfNodeIndex);
			}

			{
				auto result = global->pin_green_node(node->get_node_index());
				assert(!result.has_error());
			}

			sg.release();

			return GreenNodePin(global, node->get_node_index(), node);
		}

		struct GreenNodeDumpContext final {
		private:
			struct DumpTask {
				GreenNodeIndex src;
				wandjson::ObjectValue *dest;
				bool deep;
			};

			Global *global;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::List<DumpTask> task_list;

			wandjson::ObjectValue *root_value;

			friend class Global;

		public:
			SLKC_API GreenNodeDumpContext(Global *global, peff::Alloc *allocator, wandjson::ObjectValue *root_value);
			SLKC_API DumpResult push_task(wandjson::ObjectValue *dest, AstNodeIndex src, bool deep) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}

			SLAKE_FORCEINLINE peff::Alloc *get_allocator() const noexcept {
				return allocator.get();
			}
		};

		struct RedNode;

		using RedNodePtr = peff::SharedPtr<RedNode>;

		struct RedNode : public peff::SharedFromThis<RedNode> {
			size_t offset = 0;
			RedNodePtr parent;
			size_t parent_index = 0;
			std::variant<TokenPtr, GreenNodePin> green_node_or_token;
			peff::DynArray<RedNodePtr> children;

			SLKC_API RedNode(peff::Alloc *allocator);

			SLAKE_FORCEINLINE bool is_green_node_facade() const noexcept {
				return green_node_or_token.index() == 1;
			}
			SLAKE_FORCEINLINE bool is_token_facade() const noexcept {
				return green_node_or_token.index() == 0;
			}
			SLAKE_API GreenNodeOperationResult build_child(peff::Alloc *allocator, size_t index);
			SLAKE_API peff::Result<RedNodePtr, GreenNodeOperationResult> get_child_node(peff::Alloc *allocator, size_t index) noexcept;
			SLAKE_FORCEINLINE GreenNodePin as_green_node() const noexcept {
				return *std::get_if<GreenNodePin>(&green_node_or_token);
			}
			SLAKE_FORCEINLINE TokenPtr as_token() const noexcept {
				return *std::get_if<TokenPtr>(&green_node_or_token);
			}
		};

		SLKC_API RedNodePtr build_red_root_node(peff::Alloc *allocator, const GreenNodePin &green_node);

		SLKC_API DumpResult dump_source_token(GreenNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, const TokenPtr &token) noexcept;
		SLKC_API DumpResult dump_green_node(GreenNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, const GreenNodePin &node, bool deep) noexcept;
	}
}

#endif
