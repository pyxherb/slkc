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
			Except,
			Interface,
			Trait,

			ConstEnum,
			ScopedEnum,
			UnionEnum,

			EnumItem,
			UnionEnumItem,

			Attribute,

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

			TypeNameDef,

			Scope
		};

		class Global;

		enum class DuplicationError : uint8_t {
			NoSlot,
			OutOfMemory,
			PinningFailed
		};

		struct TypeName;

		class DuplicationContextHook {
		public:
			virtual DuplicationError run() = 0;
			virtual void dealloc() noexcept = 0;
		};

		struct DuplicationContext final {
		private:
			struct DuplicationTask {
				NodeIndex dest, src;
			};

			Global *global;
			peff::List<DuplicationTask> task_list;
			peff::List<std::unique_ptr<DuplicationContextHook, peff::DeallocableDeleter<DuplicationContextHook>>> post_run_hooks;

			friend class Global;

		public:
			SLKC_API DuplicationContext(Global *global);
			SLKC_API peff::Result<NodeIndex, DuplicationError> push_task(NodeIndex node_index) noexcept;
			SLKC_API peff::Result<TypeName, DuplicationError> push_task(const TypeName &type_name) noexcept;

			[[nodiscard]] SLKC_API bool push_post_run_hook(DuplicationContextHook *hook) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}
		};

		struct DumpContext final {
		private:
			struct DumpTask {
				NodeIndex src;
				wandjson::ObjectValue *dest;
				bool deep;
			};

			Global *global;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::List<DumpTask> task_list;

			wandjson::ObjectValue *root_value;

			friend class Global;

		public:
			SLKC_API DumpContext(Global *global, peff::Alloc *allocator, wandjson::ObjectValue *root_value);
			SLKC_API DumpResult push_task(wandjson::ObjectValue *dest, NodeIndex src, bool deep) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}

			SLAKE_FORCEINLINE peff::Alloc *get_allocator() const noexcept {
				return allocator.get();
			}
		};

		class Node {
		private:
			Global *const _global;
			TokenRange _token_range;
			const NodeType _ast_node_type;
			const NodeIndex _node_index;

		protected:
			[[nodiscard]] virtual peff::Result<Node *, DuplicationError> do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept = 0;

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			friend Global;

		public:
			SLKC_API Node(NodeType ast_node_type, Global *global, NodeIndex node_index);
			SLKC_API Node(const Node &other, DuplicationContext &context, NodeIndex node_index);
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

			PEFF_FORCEINLINE NodeIndex get_node_index() const noexcept {
				return _node_index;
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
	[[nodiscard]] SLKC_API virtual peff::Result<Node *, DuplicationError> do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept override;

/// @brief Macro used for defining a simple instance of the duplication method for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(name)                                                                                                               \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context) const noexcept {                             \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node<name>(duplication_context.get_global(), *this, duplication_context, node_index)); \
                                                                                                                                                             \
		if (!ptr)                                                                                                                                            \
			return slkc::ast::DuplicationError::OutOfMemory;                                                                                                 \
                                                                                                                                                             \
		return ptr.release();                                                                                                                                \
	}

/// @brief Macro used for defining a null instance of the duplication method for an AST node class.
#define SLKC_NULL_AST_DUPLICATE_FN_DEF(name)                                                                                     \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept { \
		peff::panic("The class " #name " cannot be duplicated");                                                                 \
		PEFF_UNREACHABLE();                                                                                                      \
	}

/// @brief Macro used for defining a simple instance of the duplication method with a result output for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(name)                                                                                                               \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept {                                         \
		peff::Option<DuplicationError> error;                                                                                                                           \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node_dup<name>(duplication_context.get_global(), *this, duplication_context, node_index, error)); \
                                                                                                                                                                         \
		if (!ptr)                                                                                                                                                        \
			return slkc::ast::DuplicationError::OutOfMemory;                                                                                                             \
                                                                                                                                                                         \
		if (error.has_value())                                                                                                                                          \
			return std::move(error).value();                                                                                                                            \
                                                                                                                                                                         \
		return static_cast<Node *>(ptr.release());                                                                                                                       \
	}

#endif
