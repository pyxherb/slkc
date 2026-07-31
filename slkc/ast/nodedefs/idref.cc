#include "idref.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API peff::Option<OwnedIdRef> OwnedIdRef::duplicate(peff::Alloc *new_allocator) const noexcept {
	OwnedIdRef new_id_ref(new_allocator);

	if (!new_id_ref.entries.resize_construct(this->entries.size(), new_allocator)) {
		return peff::NULLOPT;
	}

	const size_t len = new_id_ref.entries.size();
	for (size_t i = 0; i < len; ++i) {
		auto &ne = new_id_ref.entries.at(i);
		auto &oe = this->entries.at(i);

		ne.sti_access_op = oe.sti_access_op;
		ne.sti_name = oe.sti_name;
		ne.sti_generic_scope = oe.sti_generic_scope;
		ne.sti_left_angle_bracket = oe.sti_left_angle_bracket;
		ne.sti_right_angle_bracket = oe.sti_right_angle_bracket;

		if (!ne.name.build(oe.name))
			return peff::NULLOPT;
		if (!ne.generic_args.build(oe.generic_args))
			return peff::NULLOPT;

		if (!ne.sti_generic_args_comma_token_indices.build(oe.sti_generic_args_comma_token_indices))
			return peff::NULLOPT;
	}

	return { std::move(new_id_ref) };
}

SLKC_API DumpResult slkc::ast::dump_id_ref_entry(wandjson::ObjectValue *target_object, DumpContext &dump_context, const IdRefEntry &id_ref_entry, bool deep_dump) {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::StringValue::alloc(dump_context.get_allocator(), id_ref_entry.name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("name", v.release()))
		return DumpResult::OutOfMemory;

	if (id_ref_entry.generic_args.size()) {
		for (auto &i : id_ref_entry.generic_args) {
			v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()));

			SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, i, deep_dump));
		}
	}

	return DumpResult::Ok;
}

SLKC_API DumpResult slkc::ast::dump_id_ref(wandjson::ArrayValue *target_object, DumpContext &dump_context, const ConstIdRefView &id_ref, bool deep_dump) {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	for (const auto &i : id_ref) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;

		SLKC_RETURN_IF_DUMP_FAILED(dump_id_ref_entry(static_cast<wandjson::ObjectValue *>(v.get()), dump_context, i, deep_dump));

		if(!target_object->push_back(v.release()))
			return DumpResult::OutOfMemory;
	}

	return DumpResult::Ok;
}
