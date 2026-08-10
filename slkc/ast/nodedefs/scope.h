#ifndef _SLKC_AST_NODEDEFS_SCOPE_H_
#define _SLKC_AST_NODEDEFS_SCOPE_H_

#include "type.h"

namespace slkc {
	namespace ast {
		class MemberNode;
		class ImportNode;
		class GenericParamNode;

		struct ImplementItem {
			TypeName type;
			bool is_trait = false;

			TokenIndex sti_trait_keyword = INVALID_TOKEN_INDEX;
		};

		enum class ScopeMemberOpResult : uint8_t {
			Success = 0,
			OutOfMemory,
			PinningIOError,
		};

		class Scope {
		private:
			Global *_global;

		public:
			NodeIndex owner_node = INVALID_NODE_INDEX;

			peff::DynArray<NodePtr<MemberNode>> members;
			peff::HashMap<GlobalSharedStringRef, size_t, GlobalSharedStringRefEq> members_index;

			peff::DynArray<NodePtr<ImportNode>> anonymous_imports;

			peff::Option<TypeName> inherited_type;
			peff::DynArray<ImplementItem> implemented_types;
			
			peff::Option<TypeName> underlying_type;

			peff::DynArray<NodePtr<GenericParamNode>> generic_params;
			peff::HashMap<GlobalSharedStringRef, size_t, GlobalSharedStringRefEq> generic_params_index;

			SLKC_API Scope(NodeIndex owner_node, Global *global);
			Scope(const Scope &) = delete;
			Scope(Scope &&) = default;
			SLKC_API ~Scope();

			Scope &operator=(const Scope &) = delete;
			Scope &operator=(Scope &&) = default;

			SLKC_API peff::Result<Scope *, DuplicationError> deep_duplicate(NodeIndex new_owner_node, DuplicationContext &duplication_context);

			SLAKE_FORCEINLINE static Scope *alloc(NodeIndex owner_node, Global *global) noexcept {
				return peff::alloc_and_construct<Scope>(global->get_allocator(), alignof(Scope), owner_node, global);
			}
			SLAKE_FORCEINLINE void dealloc() noexcept {
				peff::destroy_and_release<Scope>(this->_global->get_allocator(), this, alignof(Scope));
			}

			[[nodiscard]] SLKC_API size_t push_member(NodePtr<MemberNode> member_node) noexcept;
			/// @brief Push and index a member.
			/// @param member_node Member node to be added
			/// @return Whether the member is added successfully.
			[[nodiscard]] SLKC_API ScopeMemberOpResult add_member(NodePtr<MemberNode> member_node) noexcept;
			[[nodiscard]] SLKC_API ScopeMemberOpResult index_member(size_t index_in_member_array) noexcept;
			/// @brief Remove a named member.
			/// @param name Name of the member to be removed.
			/// @return Whether the member is removed successfully.
			SLKC_API void remove_member(const std::string_view &name) noexcept;
			SLAKE_FORCEINLINE NodePtr<MemberNode> get_member(const std::string_view &name) const noexcept {
				return NodePtr<MemberNode>(_global, members.at(members_index.at_alt<std::string_view>(name)));
			}
			SLAKE_FORCEINLINE NodePtr<MemberNode> get_member(size_t index) const noexcept {
				return members.at(index);
			}
			SLAKE_FORCEINLINE NodePtr<MemberNode> try_get_member(const std::string_view &name) const noexcept {
				if (auto it = members_index.find_alt<std::string_view>(name); it != members_index.end()) {
					return members.at(it.value());
				}
				return {};
			}
			SLAKE_FORCEINLINE size_t get_member_num() const noexcept {
				return members.size();
			}
			SLAKE_FORCEINLINE size_t get_indexed_member_num() const noexcept {
				return members_index.size();
			}
			SLAKE_FORCEINLINE decltype(members)::Iterator members_begin() noexcept {
				return members.begin();
			}
			SLAKE_FORCEINLINE decltype(members)::Iterator members_end() noexcept {
				return members.end();
			}
			SLAKE_FORCEINLINE decltype(members)::ConstIterator members_begin_const() const noexcept {
				return members.cbegin();
			}
			SLAKE_FORCEINLINE decltype(members)::ConstIterator members_end_const() const noexcept {
				return members.cend();
			}
			SLAKE_FORCEINLINE const decltype(members_index) &get_members_indices() const noexcept {
				return members_index;
			}
			SLAKE_FORCEINLINE const NodePtr<MemberNode> *get_members_data() const noexcept {
				return members.data();
			}
			SLAKE_FORCEINLINE decltype(members) &get_members() noexcept {
				return members;
			}
			SLAKE_FORCEINLINE const decltype(members) &get_members_const() const noexcept {
				return members;
			}

			[[nodiscard]] SLKC_API size_t push_generic_param(NodePtr<GenericParamNode> generic_param_node) noexcept;
			[[nodiscard]] SLKC_API ScopeMemberOpResult add_generic_param(NodePtr<GenericParamNode> generic_param_node) noexcept;
			[[nodiscard]] SLKC_API ScopeMemberOpResult index_generic_param(size_t index_in_member_array) noexcept;
			SLKC_API void remove_generic_param(const std::string_view &name) noexcept;
			SLAKE_FORCEINLINE NodePtr<GenericParamNode> get_generic_param(const std::string_view &name) const noexcept {
				return generic_params.at(generic_params_index.at_alt<std::string_view>(name));
			}
			SLAKE_FORCEINLINE NodePtr<GenericParamNode> get_generic_param(size_t index) const noexcept {
				return generic_params.at(index);
			}
			SLAKE_FORCEINLINE NodePtr<GenericParamNode> try_get_generic_param(const std::string_view &name) const noexcept {
				if (auto it = generic_params_index.find_alt<std::string_view>(name); it != generic_params_index.end()) {
					return generic_params.at(it.value());
				}
				return {};
			}
			SLAKE_FORCEINLINE size_t get_generic_param_num() const noexcept {
				return generic_params.size();
			}
			SLAKE_FORCEINLINE size_t get_indexed_generic_param_num() const noexcept {
				return generic_params_index.size();
			}
			SLAKE_FORCEINLINE decltype(generic_params)::Iterator generic_params_begin() noexcept {
				return generic_params.begin();
			}
			SLAKE_FORCEINLINE decltype(generic_params)::Iterator generic_params_end() noexcept {
				return generic_params.end();
			}
			SLAKE_FORCEINLINE decltype(generic_params)::ConstIterator generic_params_begin_const() const noexcept {
				return generic_params.cbegin();
			}
			SLAKE_FORCEINLINE decltype(generic_params)::ConstIterator generic_params_end_const() const noexcept {
				return generic_params.cend();
			}
			SLAKE_FORCEINLINE const NodePtr<GenericParamNode> *get_generic_params_data() const noexcept {
				return generic_params.data();
			}
			SLAKE_FORCEINLINE decltype(generic_params) &get_generic_params() noexcept {
				return generic_params;
			}
			SLAKE_FORCEINLINE const decltype(generic_params) &get_generic_params_const() const noexcept {
				return generic_params;
			}
		};

		SLKC_API DumpResult dump_scope(wandjson::ObjectValue *target_object, DumpContext &dump_context, const Scope *scope, bool deep_dump);
	}
}

#endif
