#ifndef _SLKC_AST_NODEDEFS_MEMBER_H_
#define _SLKC_AST_NODEDEFS_MEMBER_H_

#include "scope.h"
#include <slake/access.h>
#include <slkc/ast/parser/lexer.h>

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

		class MemberNode : public AstNode {
		private:
			AstNodeIndex _parent_node_index = INVALID_AST_NODE_INDEX;
			peff::UniquePtr<Scope, peff::DeallocableDeleter<Scope>> _self_scope;

		protected:
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			GlobalSharedStringRef self_name;

			AccessModifier access_modifier;

			SLKC_API MemberNode(NodeType ast_node_type, Global *global);
			SLKC_API MemberNode(const MemberNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~MemberNode();

			SLKC_API bool alloc_scope() noexcept;

			SLAKE_FORCEINLINE Scope *get_scope() const noexcept {
				return _self_scope.get();
			}

			SLAKE_FORCEINLINE AstNodePtr<MemberNode> get_parent() const noexcept {
				return AstNodePtr<MemberNode>(get_global(), _parent_node_index);
			}

			SLAKE_FORCEINLINE void set_parent(AstNodeIndex node) noexcept {
				_parent_node_index = node;
			}

			SLAKE_FORCEINLINE bool set_name(std::string_view name) {
				assert(_parent_node_index == INVALID_AST_NODE_INDEX);
				if (!(self_name = GlobalSharedStringRef(get_global()->register_shared_string(name))))
					return false;
				return true;
			}

			SLAKE_FORCEINLINE void set_name(const GlobalSharedStringRef &name) {
				assert(_parent_node_index == INVALID_AST_NODE_INDEX);
				self_name = name;
			}

			SLAKE_FORCEINLINE GlobalSharedStringRef get_name() const noexcept {
				return self_name;
			}

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ModuleNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_module_keyword = INVALID_TOKEN_INDEX,
					   sti_module_decl_semicolon = INVALID_TOKEN_INDEX;

			peff::Option<TokenList> module_source_token_list;

			SLKC_API ModuleNode(Global *global);
			SLKC_API ModuleNode(const ModuleNode &other, DuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ModuleNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
