#ifndef _SLKC_AST_NODEDEFS_IDREF_H_
#define _SLKC_AST_NODEDEFS_IDREF_H_

#include "type_base.h"
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slkc {
	namespace ast {
		struct IdRefEntry final {
			peff::String name;
			peff::DynArray<TypeName> generic_args;

			size_t sti_access_op_token_index = SIZE_MAX,
				   sti_name_token_index = SIZE_MAX,
				   sti_generic_scope_token_index = SIZE_MAX,
				   sti_left_angle_bracket_token_index = SIZE_MAX,
				   sti_right_angle_bracket_token_index = SIZE_MAX;
			peff::DynArray<size_t> sti_generic_args_comma_token_indices;

			PEFF_FORCEINLINE IdRefEntry(peff::Alloc *self_allocator) : name(self_allocator), generic_args(self_allocator), sti_generic_args_comma_token_indices(self_allocator) {}
			PEFF_FORCEINLINE IdRefEntry(IdRefEntry &&rhs)
				: name(std::move(rhs.name)),
				  generic_args(std::move(rhs.generic_args)),
				  sti_access_op_token_index(rhs.sti_access_op_token_index),
				  sti_name_token_index(rhs.sti_name_token_index),
				  sti_left_angle_bracket_token_index(rhs.sti_left_angle_bracket_token_index),
				  sti_right_angle_bracket_token_index(rhs.sti_right_angle_bracket_token_index),
				  sti_generic_args_comma_token_indices(std::move(rhs.sti_generic_args_comma_token_indices)) {
			}
		};

		using IdRefView = std::span<IdRefEntry>;
		using ConstIdRefView = std::span<const IdRefEntry>;

		struct OwnedIdRef final {
			peff::DynArray<IdRefEntry> entries;

			PEFF_FORCEINLINE OwnedIdRef(peff::Alloc *self_allocator) : entries(self_allocator) {}
			PEFF_FORCEINLINE OwnedIdRef(OwnedIdRef &&rhs) : entries(std::move(rhs.entries)) {
			}
			OwnedIdRef &operator=(OwnedIdRef &&) = default;

			PEFF_FORCEINLINE operator IdRefView() noexcept {
				return entries;
			}

			PEFF_FORCEINLINE operator ConstIdRefView() const noexcept {
				return entries;
			}

			SLKC_API peff::Option<OwnedIdRef> duplicate(peff::Alloc *new_allocator) const noexcept;
		};

		SLKC_API DumpResult dump_id_ref_entry(wandjson::ObjectValue *target_object, DumpContext &dump_context, const IdRefEntry &id_ref_entry, bool deep_dump);
		SLKC_API DumpResult dump_id_ref(wandjson::ArrayValue *target_object, DumpContext &dump_context, const ConstIdRefView &id_ref, bool deep_dump);
	}
}

#endif
