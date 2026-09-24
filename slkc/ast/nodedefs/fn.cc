#include "fn.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(FnOverloadingNode);

SLKC_API DumpResult FnOverloadingNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_fn_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_fn_keyword", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(overloading_kind)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("overloading_kind", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), overloading_flags))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("overloading_flags", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	// Dump the parameters.
	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	auto av = static_cast<wandjson::ArrayValue *>(v.get());
	if (!target_object->insert("params", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();
	for (const auto &i : params) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(i.dump(dump_context, static_cast<wandjson::ObjectValue *>(v.get()), deep_dump));
		if (!av->push_back(v.get()))
			return DumpResult::OutOfMemory;
		discarded_v = v.release();
	}

	// Dump the return type.
	if (return_type) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("return_type", v.get()))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), return_type.get_index(), deep_dump));
		discarded_v = v.release();
	}

	if (overriden_type) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("overriden_type", v.get()))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), overriden_type.get_index(), deep_dump));
		discarded_v = v.release();
	}

	// Dump the function body (if exists).
	if (body) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		if (!target_object->insert("body", v.get()))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), body.get_index(), deep_dump));
		discarded_v = v.release();
	}

	return DumpResult::Ok;
}

SLKC_API FnOverloadingNode::FnOverloadingNode(Global *global)
	: MemberNode(NodeType::FnOverloading, global),
	  idx_param_comma_tokens(global->get_allocator()),
	  idx_generic_param_comma_tokens(global->get_allocator()),
	  params(global->get_allocator()) {
}

SLKC_API FnOverloadingNode::FnOverloadingNode(
	const FnOverloadingNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  idx_param_comma_tokens(context.get_global()->get_allocator()),
	  idx_generic_param_comma_tokens(context.get_global()->get_allocator()),
	  params(context.get_global()->get_allocator()),
	  overloading_flags(other.overloading_flags),
	  overloading_kind(other.overloading_kind),
	  sti_fn_keyword(other.sti_fn_keyword),
	  sti_name(other.sti_name),
	  sti_left_parenthesis(other.sti_left_parenthesis),
	  sti_right_parenthesis(other.sti_right_parenthesis),
	  sti_return_type_token(other.sti_return_type_token),
	  sti_vararg_token(other.sti_vararg_token),
	  sti_const_keyword(other.sti_const_keyword),
	  sti_virtual_keyword(other.sti_virtual_keyword),
	  sti_override_keyword(other.sti_override_keyword) {
	if (error_out.has_value())
		return;

	if (!params.resize(other.params.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < other.params.size(); ++i) {
		auto result = other.params[i].duplicate(context);
		if (result.is_error()) {
			error_out = std::move(result).error();
			return;
		}
		params[i] = std::move(result).value();
	}

	{
		auto element_type_result = context.push_task(other.return_type.get_index());
		if (!element_type_result.is_error()) {
			error_out = std::move(element_type_result).error();
			return;
		}
		this->return_type = AstNodePtr<TypeNameNode>(context.get_global(), std::move(element_type_result).value());
	}

	if (!idx_param_comma_tokens.build(other.idx_param_comma_tokens)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	if (!idx_generic_param_comma_tokens.build(other.idx_generic_param_comma_tokens)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	if (other.overriden_type) {
		auto overriden_type_result = context.push_task(other.overriden_type.get_index());
		if (!overriden_type_result.is_error()) {
			error_out = std::move(overriden_type_result).error();
			return;
		}
		overriden_type = AstNodePtr<TypeNameNode>(context.get_global(), std::move(overriden_type_result).value());
	}
}

SLKC_API FnOverloadingNode::~FnOverloadingNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(FnOverloadingNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(FnNode);

SLKC_API DumpResult FnNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("elements", v.get()))
			return DumpResult::OutOfMemory;
		discarded_v = v.release();

		for (const auto &i : overloadings) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), i.get_index(), deep_dump));
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			discarded_v = v.release();
		}
	}

	return DumpResult::Ok;
}

SLKC_API FnNode::FnNode(Global *global)
	: MemberNode(NodeType::Fn, global),
	  overloadings(global->get_allocator()) {
}

SLKC_API FnNode::FnNode(
	const FnNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  overloadings(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;

	if (!overloadings.resize(other.overloadings.size())) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}

	for (size_t i = 0; i < overloadings.size(); ++i) {
		auto result = context.push_task(other.overloadings[i].get_index());
		if (result.is_error()) {
			error_out = std::move(result).error();
			return;
		}
		overloadings[i] = AstNodePtr<FnOverloadingNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API FnNode::~FnNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(FnNode);
