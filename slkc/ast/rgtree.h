#ifndef _SLKC_AST_RGTREE_H_
#define _SLKC_AST_RGTREE_H_

#include "global.h"
#include "parser/lexer.h"
#include <peff/advutils/shared_ptr.h>
#include <peff/containers/btree_map.h>

namespace slkc {
	namespace ast {
		namespace RGNodeKind {
			enum {
				/// @brief Invalid node type.
				Invalid = 0x10000000,

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

				/// @brief A unary expression.
				UnaryExpr,

				/// @brief A binary expression.
				BinaryExpr,

				/// @brief A ternary expression.
				TernaryExpr,

				/// @brief An identifier reference expression.
				IdRefExpr,

				/// @brief An identifier reference expression with a preceding expression.
				HeadedIdRefExpr,

				/// @brief An i8-typed integer literal expression.
				I8LiteralExpr,

				/// @brief An i16-typed integer literal expression.
				I16LiteralExpr,

				/// @brief An i32-typed integer literal expression.
				I32LiteralExpr,

				/// @brief An i64-typed integer literal expression.
				I64LiteralExpr,

				/// @brief An u8-typed integer literal expression.
				U8LiteralExpr,

				/// @brief An u16-typed integer literal expression.
				U16LiteralExpr,

				/// @brief An u32-typed integer literal expression.
				U32LiteralExpr,

				/// @brief An u64-typed integer literal expression.
				U64LiteralExpr,

				/// @brief An f32-typed floating-point literal expression.
				F32LiteralExpr,

				/// @brief An f64-typed floating-point literal expression.
				F64LiteralExpr,

				/// @brief A string literal expression.
				StringLiteralExpr,

				/// @brief A boolean expression.
				BoolLiteralExpr,

				/// @brief A null expression.
				NullLiteralExpr,

				/// @brief An initializer list expression.
				InitializerListExpr,

				/// @brief A function call expression.
				CallExpr,

				/// @brief A new operation expression.
				NewExpr,

				/// @brief A type-casting expression.
				CastExpr,

				/// @brief A match expression.
				MatchExpr,
				/// @brief A match case.
				MatchCase,

				/// @brief A grouping (parenthesized) expression.
				GroupExpr,

				/// @brief An argument passing to a function.
				Arg,

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

				/// @brief An if statement.
				IfStmt,

				/// @brief A for statement.
				ForStmt,

				/// @brief A while statement.
				WhileStmt,

				/// @brief A do-while statement.
				DoWhileStmt,

				/// @brief A let statement.
				LetStmt,

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
				/// @brief A case of a switch statement.
				SwitchCase,
				/// @brief Default case of a switch statement.
				SwitchDefaultCase,

				/// @brief An expression statement.
				ExprStmt,

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
				/// @brief A constant enum definition.
				ConstEnumDef,
				/// @brief A scoped enum definition.
				ScopedEnumDef,
				/// @brief An union enum definition.
				UnionEnumDef,
				/// @brief An attribute definition.
				AttributeDef,

				/// @brief An import item.
				ImportItem,

				Max
			};
		}

		SLAKE_FORCEINLINE bool is_token_node_kind(TokenKind kind) noexcept {
			return kind >= static_cast<uint32_t>(TokenId::End) && kind < static_cast<uint32_t>(TokenId::MaxToken);
		}

		SLAKE_FORCEINLINE bool is_rg_node_kind(TokenKind kind) noexcept {
			return kind >= static_cast<uint32_t>(RGNodeKind::Invalid) && kind < static_cast<uint32_t>(RGNodeKind::Max);
		}

		struct RGNode;

		class RGNodePin final {
		private:
			using ThisType = RGNodePin;
			Global *_global;
			RGNodeIndex _rg_node_index;
			union {
				RGNode *_ptr;
				PinFailReason _fail_reason;
			};

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_rg_node_index = node_index;
				global->pin_rg_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_ptr)
					_global->unpin_rg_node(_rg_node_index);
			}

			SLAKE_FORCEINLINE RGNodePin() : _global(nullptr), _rg_node_index(INVALID_NODE_INDEX), _ptr(nullptr) {}
			SLAKE_FORCEINLINE explicit RGNodePin(Global *global, NodeIndex node_index, RGNode *ptr) : _global(global), _rg_node_index(node_index), _ptr(ptr) {
			}
			SLAKE_FORCEINLINE explicit RGNodePin(NodeIndex node_index, PinFailReason reason) : _global(nullptr), _rg_node_index(node_index), _fail_reason(reason) {
			}
			SLAKE_FORCEINLINE ~RGNodePin() {
				reset();
			}
			SLAKE_FORCEINLINE RGNodePin(const ThisType &rhs) noexcept : _global(rhs._global), _rg_node_index(rhs._rg_node_index) {
				_global->pin_rg_node(_rg_node_index);
			}
			SLAKE_FORCEINLINE RGNodePin(ThisType &&rhs) noexcept : _global(rhs._global), _rg_node_index(rhs._rg_node_index) {
				rhs._rg_node_index = INVALID_NODE_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._rg_node_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_rg_node_index = rhs._rg_node_index;
				rhs._rg_node_index = INVALID_NODE_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE RGNode *get() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE NodeIndex get_index() const noexcept {
				return _rg_node_index;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE RGNode *operator->() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_rg_node_index > rhs._rg_node_index)
					return 1;
				if (_rg_node_index < rhs._rg_node_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _rg_node_index < rhs._rg_node_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _rg_node_index > rhs._rg_node_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _rg_node_index == rhs._rg_node_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _rg_node_index != rhs._rg_node_index;
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

		class RGNodePtr final {
		private:
			using ThisType = RGNodePtr;
			Global *_global;
			NodeIndex _node_index;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->ref_rg_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_node_index != INVALID_NODE_INDEX)
					_global->unref_rg_node(_node_index);
			}

			SLAKE_FORCEINLINE RGNodePtr() : _global(nullptr), _node_index(INVALID_NODE_INDEX) {}
			SLAKE_FORCEINLINE RGNodePtr(const RGNodePin &pin) : _global(pin.get_global()), _node_index(pin.get_index()) {}
			SLAKE_FORCEINLINE explicit RGNodePtr(Global *global, NodeIndex node_index) : _global(global), _node_index(node_index) {
				global->ref_rg_node(node_index);
			}
			SLAKE_FORCEINLINE ~RGNodePtr() {
				reset();
			}

			SLAKE_FORCEINLINE RGNodePtr(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->ref_rg_node(_node_index);
			}
			SLAKE_FORCEINLINE RGNodePtr(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_NODE_INDEX;
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
				rhs._node_index = INVALID_NODE_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE NodeIndex get_index() const noexcept {
				return _node_index;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			SLAKE_FORCEINLINE RGNodePin pin() const noexcept {
				auto result = _global->pin_rg_node(_node_index);
				if (result.has_error())
					return RGNodePin(_node_index, std::move(result).error());
				return RGNodePin(_global, _node_index, std::move(result).value());
			}

			SLAKE_FORCEINLINE static RGNodePtr from_pin(RGNodePin pin) noexcept {
				return RGNodePtr(pin);
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
				return _node_index != INVALID_NODE_INDEX;
			}
		};

		using RGNodeChildList = peff::DynArray<RGNodePtr>;
		using RGNodeIndexMap = peff::HashMap<uint32_t, size_t>;

		struct RGNode {
		private:
			Global *_global;
			NodeIndex _node_index;

			friend class Global;

		public:
			TokenPtr source_token;
			RGNodeChildList children;
			TokenKind node_kind;

			SLKC_API RGNode(Global *global);
			SLKC_API ~RGNode();

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE NodeIndex get_node_index() const noexcept {
				return _node_index;
			}

			SLAKE_FORCEINLINE void set_node_index(NodeIndex index) noexcept {
				_node_index = index;
			}

			SLAKE_FORCEINLINE bool push_child(RGNodePtr child) noexcept {
				return children.push_back(std::move(child));
			}

			SLKC_API bool index_children() noexcept;
		};

		SLAKE_FORCEINLINE RGNodePin make_rg_node(Global *global) {
			RGNode *node = peff::alloc_and_construct<RGNode>(global->get_allocator(), alignof(RGNode), global);
			if (!node)
				return RGNodePin();
			peff::ScopeGuard sg([global, node]() noexcept {
				peff::destroy_and_release<RGNode>(global->get_allocator(), node, alignof(RGNode));
			});
			if (!global->map_rg_node(node).has_value())
				return RGNodePin();
			sg.release();

			global->pin_rg_node(node->get_node_index());

			return RGNodePin(global, node->get_node_index(), node);
		}
	}
}

#endif
