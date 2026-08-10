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
			NodeIndex _node_index = INVALID_NODE_INDEX;

		protected:
			[[nodiscard]] virtual peff::Result<Node *, DuplicationError> do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept = 0;

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			friend Global;

		public:
			SLKC_API Node(NodeType ast_node_type, Global *global);
			SLKC_API Node(const Node &other, DuplicationContext &context, NodeIndex node_index);
			SLKC_API virtual ~Node();

			virtual void dealloc() noexcept = 0;

			SLAKE_FORCEINLINE NodeType get_ast_node_type() const noexcept {
				return _ast_node_type;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE TokenRange get_token_range() const noexcept {
				return _token_range;
			}

			SLAKE_FORCEINLINE void set_token_range(TokenRange token_range) noexcept {
				_token_range = token_range;
			}
			
			SLAKE_FORCEINLINE void set_end_token_index(TokenIndex end_token_index) noexcept {
				_token_range.end = end_token_index;
			}

			SLAKE_FORCEINLINE NodeIndex get_node_index() const noexcept {
				return _node_index;
			}
			
			SLAKE_FORCEINLINE void set_node_index(NodeIndex node_index) noexcept {
				_node_index = node_index;
			}
		};
	}
}

#endif
