#include "type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ArrayTypeDefNode);

[[nodiscard]] SLKC_API DumpResult ArrayTypeDefNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *value_out, bool deep_dump) const noexcept {
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
