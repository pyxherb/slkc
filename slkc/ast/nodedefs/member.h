#ifndef _SLKC_AST_NODEDEFS_MEMBER_H_
#define _SLKC_AST_NODEDEFS_MEMBER_H_

#include "scope.h"
#include <slake/access.h>

namespace slkc {
	namespace ast {
		enum class Visibility {
			Unspecified = 0,
			Public,
			Private,
			Protected
		};

		struct AccessModifier {
			Visibility visibility = Visibility::Unspecified;
			bool is_static = false;
			bool is_native = false;

			/// @brief Token index to the visibility keyword (public, private, protected, etc).
			TokenIndex sti_visibility_token = INVALID_TOKEN_INDEX;
			TokenIndex sti_static_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_native_keyword = INVALID_TOKEN_INDEX;
		};

		class MemberNode : public Node {
		protected:
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			GlobalSharedStringRef self_name;
			std::unique_ptr<Scope, peff::DeallocableDeleter<Scope>> self_scope;

			AccessModifier access_modifier;

			SLKC_API MemberNode(NodeType ast_node_type, Global *global, NodeIndex node_index);
			SLKC_API MemberNode(const MemberNode &other, DuplicationContext &context, NodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~MemberNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
