#include "scope.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API Scope::Scope(NodeIndex owner_node, Global *global)
	: _global(global),
	  owner_node(owner_node),
	  members(global->get_allocator()),
	  members_index(global->get_allocator()),
	  anonymous_imports(global->get_allocator()),
	  implemented_types(global->get_allocator()),
	  generic_params(global->get_allocator()),
	  generic_params_index(global->get_allocator()) {
}

SLKC_API Scope::~Scope() {
}

SLKC_API peff::Result<Scope *, DuplicationError> Scope::deep_duplicate(NodeIndex new_owner_node, DuplicationContext &duplication_context) {
	std::unique_ptr<Scope, peff::DeallocableDeleter<Scope>> new_scope(Scope::alloc(new_owner_node, _global));

	if (!new_scope)
		return DuplicationError::OutOfMemory;

	// Duplicate members.
	{
		if (!new_scope->members.resize(this->members.size()))
			return DuplicationError::OutOfMemory;

		const size_t limit = members.size();
		for (size_t i = 0; i < limit; ++i) {
			auto result = duplication_context.push_task(members[i]);
			if (!result.has_error())
				return std::move(result).error();
			new_scope->members[i] = NodePtr<MemberNode>(duplication_context.get_global(), std::move(result).value());
		}
	}

	// Duplicate members index.
	for (auto [k, v] : members_index) {
		if (!new_scope->members_index.insert(GlobalSharedStringRef(k), +v))
			return DuplicationError::OutOfMemory;
	}

	// Duplicate anonymous imports.
	{
		if (!new_scope->anonymous_imports.resize(this->anonymous_imports.size()))
			return DuplicationError::OutOfMemory;

		const size_t limit = anonymous_imports.size();
		for (size_t i = 0; i < limit; ++i) {
			auto result = duplication_context.push_task(anonymous_imports[i]);
			if (!result.has_error())
				return std::move(result).error();
			new_scope->anonymous_imports[i] = NodePtr<ImportNode>(duplication_context.get_global(), std::move(result).value());
		}
	}

	// Duplicate inherited type.
	if (inherited_type.has_value()) {
		auto r = duplication_context.push_task(*inherited_type);
		if (r.has_error())
			return std::move(r).error();
		new_scope->inherited_type = std::move(r).value();
	}

	// Duplicate implemented types.
	{
		if (!new_scope->implemented_types.resize(this->implemented_types.size()))
			return DuplicationError::OutOfMemory;

		const size_t limit = implemented_types.size();
		for (size_t i = 0; i < limit; ++i) {
			new_scope->implemented_types[i] = implemented_types[i];

			auto result = duplication_context.push_task(implemented_types[i].type);
			if (!result.has_error())
				return std::move(result).error();
			new_scope->implemented_types[i].type = std::move(result).value();
		}
	}

	// Duplicate generic parameters.
	{
		if (!new_scope->generic_params.resize(this->generic_params.size()))
			return DuplicationError::OutOfMemory;

		const size_t limit = generic_params.size();
		for (size_t i = 0; i < limit; ++i) {
			auto result = duplication_context.push_task(generic_params[i]);
			if (!result.has_error())
				return std::move(result).error();
			new_scope->generic_params[i] = NodePtr<GenericParamNode>(duplication_context.get_global(), std::move(result).value());
		}
	}

	// Duplicate generic_params index.
	for (auto [k, v] : generic_params_index) {
		if (!new_scope->generic_params_index.insert(GlobalSharedStringRef(k), +v))
			return DuplicationError::OutOfMemory;
	}

	return new_scope.release();
}

SLKC_API DumpResult slkc::ast::dump_scope(wandjson::ObjectValue *target_object, DumpContext &dump_context, const Scope *scope, bool deep_dump) {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;

	{
		wandjson::ArrayValue *members_array = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("members", v.release()))
			return DumpResult::OutOfMemory;
		for (auto &member : scope->members) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			wandjson::ObjectValue *ov = static_cast<wandjson::ObjectValue *>(v.get());
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(ov, member.get_index(), deep_dump));
			if (!members_array->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	{
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("members_index", v.release()))
			return DumpResult::OutOfMemory;
		wandjson::ObjectValue *members_index = static_cast<wandjson::ObjectValue *>(v.get());
		for (auto [name, index] : scope->members_index) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), index))))
				return DumpResult::OutOfMemory;
			if (!members_index->insert(name, v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	{
		wandjson::ArrayValue *anon_imports_array = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("members", v.release()))
			return DumpResult::OutOfMemory;
		for (auto &member : scope->anonymous_imports) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			wandjson::ObjectValue *ov = static_cast<wandjson::ObjectValue *>(v.get());
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(ov, member.get_index(), deep_dump));
			if (!anon_imports_array->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (scope->inherited_type.has_value()) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ObjectValue *ov = static_cast<wandjson::ObjectValue *>(v.get());
		if (!target_object->insert("inherited_type", v.release()))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_typename(ov, dump_context, scope->inherited_type.value(), deep_dump));
	}

	for (size_t i = 0; i < scope->implemented_types.size(); ++i) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ObjectValue *ov = static_cast<wandjson::ObjectValue *>(v.get());
		if (!target_object->insert("implemented_types", v.release()))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_typename(ov, dump_context, scope->implemented_types[i].type, deep_dump));
	}

	{
		wandjson::ArrayValue *members_array = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("generic_params", v.release()))
			return DumpResult::OutOfMemory;
		for (auto &member : scope->generic_params) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			wandjson::ObjectValue *ov = static_cast<wandjson::ObjectValue *>(v.get());
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(ov, member.get_index(), deep_dump));
			if (!members_array->push_back(v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	{
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("generic_params_index", v.release()))
			return DumpResult::OutOfMemory;
		wandjson::ObjectValue *generic_params_index = static_cast<wandjson::ObjectValue *>(v.get());
		for (auto [name, index] : scope->generic_params_index) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), index))))
				return DumpResult::OutOfMemory;
			if (!generic_params_index->insert(name, v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}
