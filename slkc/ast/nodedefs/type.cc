#include "type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(CustomTypeDefNode);

[[nodiscard]] SLKC_API DumpResult CustomTypeDefNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_id_ref(static_cast<wandjson::ArrayValue*>(v.get()), dump_context, referred_name, deep_dump));
	if (!target_object->insert("referred_name", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API CustomTypeDefNode::CustomTypeDefNode(Global *global)
	: Node(NodeType::TypeNameDef, global), referred_name(global->get_allocator()) {
}

SLKC_API CustomTypeDefNode::CustomTypeDefNode(
	const CustomTypeDefNode &other,
	DuplicationContext &context,
	peff::Option<DuplicationResult> &result_out)
	: Node(other, context),
	  referred_name(context.get_global()->get_allocator()) {
	{
		auto result = other.referred_name.duplicate(context.get_global()->get_allocator());

		if (!result.has_value()) {
			result_out = DuplicationResult::OutOfMemory;
			return;
		}

		referred_name = std::move(result).value();
	}
}

SLKC_API CustomTypeDefNode::~CustomTypeDefNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(CustomTypeDefNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ArrayTypeDefNode);

[[nodiscard]] SLKC_API DumpResult ArrayTypeDefNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_typename(static_cast<wandjson::ObjectValue*>(v.get()), dump_context, element_type, deep_dump));
	if (!target_object->insert("element_type", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ArrayTypeDefNode::ArrayTypeDefNode(Global *global)
	: Node(NodeType::TypeNameDef, global) {
}

SLKC_API ArrayTypeDefNode::ArrayTypeDefNode(
	const ArrayTypeDefNode &other,
	DuplicationContext &context,
	peff::Option<DuplicationResult> &result_out)
	: Node(other, context) {
	{
		auto element_type_result = context.push_task(other.element_type);
		if (!element_type_result.has_error()) {
			result_out = std::move(element_type_result).error();
			return;
		}
		this->element_type = std::move(element_type_result).value();
	}
}

SLKC_API bool DuplicationContext::push_post_run_hook(DuplicationContextHook *hook) noexcept {
	if(!post_run_hooks.push_back(std::unique_ptr<DuplicationContextHook, peff::DeallocableDeleter<DuplicationContextHook>>(hook)))
		return false;
	return true;
}

SLKC_API ArrayTypeDefNode::~ArrayTypeDefNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ArrayTypeDefNode);
