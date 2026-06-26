#ifndef _SLKC_AST_NODE_H_
#define _SLKC_AST_NODE_H_

#include "basedefs.h"
#include <peff/utils/result.h>
#include <peff/containers/map.h>
#include <peff/containers/hashmap.h>
#include <peff/base/deallocable.h>
#include <wandjson/parser.h>
#include <wandjson/dump.h>
#include <config.h>

namespace slkc {
	namespace ast {
		enum class NodeType : uint8_t {
			Bad = 0,

			Class,

			Struct,

			ConstEnum,
			ScopedEnum,
			UnionEnum,

			EnumItem,
			UnionEnumItem,

			Attribute,

			Except,
			Interface,

			AppliedAttribute,

			Fn,
			FnOverloading,

			Stmt,
			Expr,

			Var,

			GenericParam,

			Module,

			Import,

			This,

			TypeNameDef
		};

		class Global;

		enum class DuplicationResult : uint8_t {
			NoSlot,
			OutOfMemory,
			PinningFailed
		};

		struct TypeName;

		struct DuplicationContext {
		private:
			struct DuplicationTask {
				NodeIndex dest, src;
			};

			Global *global;
			peff::List<DuplicationTask> task_list;

			friend class Global;

		public:
			SLKC_API DuplicationContext(Global *global);
			SLKC_API peff::Option<NodeIndex> push_task(NodeIndex node_index) noexcept;
			SLKC_API peff::Option<TypeName> push_task(const TypeName &type_name) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}
		};

		struct DumpContext {
		private:
			struct DumpTask {
				NodeIndex src;
				wandjson::ObjectValue *dest;
			};

			Global *global;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::List<DumpTask> task_list;

			wandjson::ObjectValue *root_value;

			friend class Global;

		public:
			SLKC_API DumpContext(Global *global, peff::Alloc *allocator, wandjson::ObjectValue *root_value);
			SLKC_API bool push_task(wandjson::ObjectValue *dest, NodeIndex src) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}

			SLAKE_FORCEINLINE peff::Alloc *get_allocator() const noexcept {
				return allocator.get();
			}
		};

		class Node {
		private:
			const NodeType _ast_node_type;
			Global *const _global;
			TokenRange _token_range;

		protected:
			[[nodiscard]] virtual peff::Result<Node *, DuplicationResult> do_duplicate(DuplicationContext &duplication_context) const noexcept = 0;

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *value_out, bool deep_dump) const noexcept;

			friend Global;

		public:
			SLKC_API Node(NodeType ast_node_type, Global *global);
			SLKC_API Node(const Node &other, DuplicationContext &context);
			SLKC_API virtual ~Node();

			virtual void dealloc() noexcept = 0;

			PEFF_FORCEINLINE NodeType get_ast_node_type() const noexcept {
				return _ast_node_type;
			}

			PEFF_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			PEFF_FORCEINLINE TokenRange get_token_range() const noexcept {
				return _token_range;
			}

			PEFF_FORCEINLINE void set_token_range(TokenRange token_range) noexcept {
				_token_range = token_range;
			}
		};
	}
}

/// @brief Macro used for declaring a simple instance of the deallocation method for an AST node class.
#define SLKC_SIMPLE_AST_DEALLOC_FN_DECL() \
	SLKC_API void dealloc() noexcept override

/// @brief Macro used for defining a simple instance of the deallocation method for an AST node class.
#define SLKC_SIMPLE_AST_DEALLOC_FN_DEF(name)                                                       \
	SLKC_API void name::dealloc() noexcept {                                                       \
		peff::destroy_and_release<name>(this->get_global()->get_allocator(), this, alignof(name)); \
	}

/// @brief Macro used for declaring a simple instance of the duplication method for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DECL() \
	[[nodiscard]] SLKC_API virtual peff::Result<Node *, DuplicationResult> do_duplicate(DuplicationContext &duplication_context) const noexcept override;

/// @brief Macro used for defining a simple instance of the duplication method for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(name)                                                                                                               \
	SLKC_API peff::Result<Node *, DuplicationResult> name::do_duplicate(DuplicationContext &duplication_context) const noexcept {                            \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node<name>(duplication_context.get_global(), *this, duplication_context)); \
                                                                                                                                                             \
		if (!ptr)                                                                                                                                            \
			return slkc::ast::DuplicationResult::OutOfMemory;                                                                                                \
                                                                                                                                                             \
		return ptr.release();                                                                                                                                \
	}

/// @brief Macro used for defining a simple instance of the duplication method with a result output for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(name)                                                                                                               \
	SLKC_API peff::Result<Node *, DuplicationResult> name::do_duplicate(DuplicationContext &duplication_context) const noexcept {                                        \
		peff::Option<DuplicationResult> result;                                                                                                                          \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node_dup<name>(duplication_context.get_global(), *this, duplication_context, result)); \
                                                                                                                                                                         \
		if (!ptr)                                                                                                                                                        \
			return slkc::ast::DuplicationResult::OutOfMemory;                                                                                                            \
                                                                                                                                                                         \
		if (result.has_value())                                                                                                                                          \
			return std::move(result).value();                                                                                                                            \
                                                                                                                                                                         \
		return static_cast<Node *>(ptr.release());                                                                                                                       \
	}

#endif
