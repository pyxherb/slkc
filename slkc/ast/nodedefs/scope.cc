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

SLKC_API peff::Result<Scope *, DuplicationResult> Scope::deep_duplicate(NodeIndex new_owner_node, DuplicationContext &duplication_context) {
	std::unique_ptr<Scope, peff::DeallocableDeleter<Scope>> new_scope(Scope::alloc(new_owner_node, _global));

	if (!new_scope)
		return DuplicationResult::OutOfMemory;

	// Duplicate members.
	{
		if (!new_scope->members.resize(this->members.size()))
			return DuplicationResult::OutOfMemory;

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
			return DuplicationResult::OutOfMemory;
	}

	// Duplicate anonymous imports.
	{
		if (!new_scope->anonymous_imports.resize(this->anonymous_imports.size()))
			return DuplicationResult::OutOfMemory;

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
			return DuplicationResult::OutOfMemory;

		const size_t limit = implemented_types.size();
		for (size_t i = 0; i < limit; ++i) {
			auto result = duplication_context.push_task(implemented_types[i]);
			if (!result.has_error())
				return std::move(result).error();
			new_scope->implemented_types[i] = std::move(result).value();
		}
	}

	// Duplicate generic parameters.
	{
		if (!new_scope->generic_params.resize(this->generic_params.size()))
			return DuplicationResult::OutOfMemory;

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
			return DuplicationResult::OutOfMemory;
	}

	return new_scope.release();
}

SLKC_API DumpResult slkc::ast::dump_scope(wandjson::ObjectValue *target_object, DumpContext &dump_context, const Scope *scope, bool deep_dump) {
}
