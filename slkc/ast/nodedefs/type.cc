#include "type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(CustomTypeDefNode);

[[nodiscard]] SLKC_API DumpResult CustomTypeDefNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	// TODO: Implement it.
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

		if(!result.has_value()) {
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
	// TODO: Implement it.
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
		if (!element_type_result.has_value()) {
			result_out = DuplicationResult::OutOfMemory;
			return;
		}
		this->element_type = *element_type_result;
	}
}

SLKC_API ArrayTypeDefNode::~ArrayTypeDefNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ArrayTypeDefNode);
