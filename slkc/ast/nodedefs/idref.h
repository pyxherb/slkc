#ifndef _SLKC_AST_NODEDEFS_IDREF_H_
#define _SLKC_AST_NODEDEFS_IDREF_H_

#include "type_base.h"
#include "../utils.h"
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slkc {
	namespace ast {
		struct IdRefEntry final {
			GlobalSharedStringRef name;
			peff::DynArray<AstNodePtr<TypeNameNode>> generic_args;

			TokenIndex sti_access_op = INVALID_TOKEN_INDEX,
				   sti_name = INVALID_TOKEN_INDEX,
				   sti_generic_scope = INVALID_TOKEN_INDEX,
				   sti_left_angle_bracket = INVALID_TOKEN_INDEX,
				   sti_right_angle_bracket = INVALID_TOKEN_INDEX;
			peff::DynArray<size_t> sti_generic_args_comma_token_indices;

			SLAKE_FORCEINLINE IdRefEntry(peff::Alloc *self_allocator) : generic_args(self_allocator), sti_generic_args_comma_token_indices(self_allocator) {}
			SLAKE_FORCEINLINE IdRefEntry(IdRefEntry &&rhs)
				: name(std::move(rhs.name)),
				  generic_args(std::move(rhs.generic_args)),
				  sti_access_op(rhs.sti_access_op),
				  sti_name(rhs.sti_name),
				  sti_left_angle_bracket(rhs.sti_left_angle_bracket),
				  sti_right_angle_bracket(rhs.sti_right_angle_bracket),
				  sti_generic_args_comma_token_indices(std::move(rhs.sti_generic_args_comma_token_indices)) {
			}
		};

		using IdRefView = std::span<IdRefEntry>;
		using ConstIdRefView = std::span<const IdRefEntry>;

		struct OwnedIdRef final {
			peff::DynArray<IdRefEntry> entries;

			SLAKE_FORCEINLINE OwnedIdRef(peff::Alloc *self_allocator) : entries(self_allocator) {}
			SLAKE_FORCEINLINE OwnedIdRef(OwnedIdRef &&rhs) : entries(std::move(rhs.entries)) {
			}
			OwnedIdRef &operator=(OwnedIdRef &&) = default;

			SLAKE_FORCEINLINE operator IdRefView() noexcept {
				return entries;
			}

			SLAKE_FORCEINLINE operator ConstIdRefView() const noexcept {
				return entries;
			}

			SLKC_API peff::Option<OwnedIdRef> duplicate(peff::Alloc *new_allocator) const noexcept;
		};

		SLKC_API DumpResult dump_id_ref_entry(wandjson::ObjectValue *target_object, AstNodeDumpContext &dump_context, const IdRefEntry &id_ref_entry, bool deep_dump);
		SLKC_API DumpResult dump_id_ref(wandjson::ArrayValue *target_object, AstNodeDumpContext &dump_context, const ConstIdRefView &id_ref, bool deep_dump);
	}
}

#endif
