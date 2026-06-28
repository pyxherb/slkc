#ifndef _SLKC_AST_NODEDEFS_SCOPE_H_
#define _SLKC_AST_NODEDEFS_SCOPE_H_

#include "type.h"

namespace slkc {
	namespace ast {
		class MemberNode;
		class ImportNode;
		class GenericParamNode;

		class Scope {
		private:
			Global *_global;

		public:
			NodeIndex owner_node = INVALID_NODE_INDEX;

			peff::DynArray<NodePtr<MemberNode>> members;
			peff::HashMap<GlobalSharedStringRef, size_t> members_index;

			peff::DynArray<NodePtr<ImportNode>> anonymous_imports;

			peff::Option<TypeName> inherited_type;
			peff::DynArray<TypeName> implemented_types;

			peff::DynArray<NodePtr<GenericParamNode>> generic_params;
			peff::HashMap<GlobalSharedStringRef, size_t> generic_params_index;

			SLKC_API Scope(NodeIndex owner_node, Global *global);
			Scope(const Scope &) = delete;
			Scope(Scope &&) = default;
			SLKC_API ~Scope();

			Scope &operator=(const Scope &) = delete;
			Scope &operator=(Scope &&) = default;

			SLKC_API peff::Result<Scope *, DuplicationResult> deep_duplicate(NodeIndex new_owner_node, DuplicationContext &duplication_context);

			SLAKE_FORCEINLINE static Scope *alloc(NodeIndex owner_node, Global *global) noexcept {
				return peff::alloc_and_construct<Scope>(global->get_allocator(), alignof(Scope), owner_node, global);
			}
			SLAKE_FORCEINLINE void dealloc() noexcept {
				peff::destroy_and_release<Scope>(this->_global->get_allocator(), this, alignof(Scope));
			}
		};

		SLKC_API DumpResult dump_scope(wandjson::ObjectValue *target_object, DumpContext &dump_context, const Scope *scope, bool deep_dump);
	}
}

#endif
