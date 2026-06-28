#ifndef _SLKC_AST_NODEDEFS_MEMBER_H_
#define _SLKC_AST_NODEDEFS_MEMBER_H_

#include "scope.h"

namespace slkc {
	namespace ast {
		class MemberNode : public Node {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			GlobalSharedStringRef self_name;
			std::unique_ptr<Scope, peff::DeallocableDeleter<Scope>> self_scope;

			SLKC_API MemberNode(NodeType ast_node_type, Global *global);
			SLKC_API MemberNode(const MemberNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~MemberNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
