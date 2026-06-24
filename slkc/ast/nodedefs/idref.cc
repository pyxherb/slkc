#include "idref.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API peff::Option<OwnedIdRef> OwnedIdRef::duplicate(peff::Alloc *new_allocator) const noexcept {
	OwnedIdRef new_id_ref(new_allocator);

	if (!new_id_ref.entries.resize_construct(this->entries.size(), new_allocator)) {
		return peff::NULL_OPTION;
	}

	const size_t len = new_id_ref.entries.size();
	for (size_t i = 0; i < len; ++i) {
		auto &ne = new_id_ref.entries.at(i);
		auto &oe = this->entries.at(i);

		ne.sti_access_op_token_index = oe.sti_access_op_token_index;
		ne.sti_name_token_index = oe.sti_name_token_index;
		ne.sti_generic_scope_token_index = oe.sti_generic_scope_token_index;
		ne.sti_left_angle_bracket_token_index = oe.sti_left_angle_bracket_token_index;
		ne.sti_right_angle_bracket_token_index = oe.sti_right_angle_bracket_token_index;

		if (!ne.name.build(oe.name))
			return peff::NULL_OPTION;
		if (!ne.generic_args.build(oe.generic_args))
			return peff::NULL_OPTION;

		if (!ne.sti_generic_args_comma_token_indices.build(oe.sti_generic_args_comma_token_indices))
			return peff::NULL_OPTION;
	}

	return { std::move(new_id_ref) };
}
