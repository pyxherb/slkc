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

		class AstNodeDuplicationContextHook {
		public:
			virtual DuplicationError run() = 0;
			virtual void dealloc() noexcept = 0;
		};

		struct AstNodeDuplicationContext final {
		private:
			struct DuplicationTask {
				AstNodeIndex dest, src;
			};

			Global *global;
			peff::List<DuplicationTask> task_list;
			peff::List<std::unique_ptr<AstNodeDuplicationContextHook, peff::DeallocableDeleter<AstNodeDuplicationContextHook>>> post_run_hooks;

			friend class Global;

		public:
			SLKC_API AstNodeDuplicationContext(Global *global);
			SLKC_API peff::Result<AstNodeIndex, DuplicationError> push_task(AstNodeIndex node_index) noexcept;
			SLKC_API peff::Result<TypeName, DuplicationError> push_task(const TypeName &type_name) noexcept;

			[[nodiscard]] SLKC_API bool push_post_run_hook(AstNodeDuplicationContextHook *hook) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}
		};

		struct AstNodeDumpContext final {
		private:
			struct DumpTask {
				AstNodeIndex src;
				wandjson::ObjectValue *dest;
				bool deep;
			};

			Global *global;
			peff::RcObjectPtr<peff::Alloc> allocator;
			peff::List<DumpTask> task_list;

			wandjson::ObjectValue *root_value;

			friend class Global;

		public:
			SLKC_API AstNodeDumpContext(Global *global, peff::Alloc *allocator, wandjson::ObjectValue *root_value);
			SLKC_API DumpResult push_task(wandjson::ObjectValue *dest, AstNodeIndex src, bool deep) noexcept;

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}

			SLAKE_FORCEINLINE peff::Alloc *get_allocator() const noexcept {
				return allocator.get();
			}
		};

		class AstNode {
		private:
			AstNode *_next_destructible = nullptr;
			Global *const _global;
			const NodeType _ast_node_type;
			AstNodeIndex _node_index = INVALID_AST_NODE_INDEX;

		protected:
			[[nodiscard]] virtual peff::Result<AstNode *, DuplicationError> do_duplicate(AstNodeDuplicationContext &duplication_context, AstNodeIndex node_index) const noexcept = 0;

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept;

			friend Global;

		public:
			SLKC_API AstNode(NodeType ast_node_type, Global *global);
			SLKC_API AstNode(const AstNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~AstNode();

			virtual void dealloc() noexcept = 0;

			SLAKE_FORCEINLINE NodeType get_ast_node_type() const noexcept {
				return _ast_node_type;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE AstNodeIndex get_node_index() const noexcept {
				return _node_index;
			}

			SLAKE_FORCEINLINE void set_node_index(AstNodeIndex node_index) noexcept {
				_node_index = node_index;
			}
		};
	}
}

#endif
