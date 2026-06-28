#include "member.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(MemberNode);

[[nodiscard]] SLKC_API DumpResult MemberNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(Node::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(dump_string(dump_context, self_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("self_name", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_scope(static_cast<wandjson::ObjectValue*>(v.get()), dump_context, self_scope.get(),deep_dump));
	if (!target_object->insert("self_scope", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API MemberNode::MemberNode(Global *global)
	: Node(NodeType::TypeNameDef, global) {
}

SLKC_API MemberNode::MemberNode(
	const MemberNode &other,
	DuplicationContext &context,
	peff::Option<DuplicationResult> &result_out)
	: Node(other, context),
	  self_name(other.self_name) {
}

SLKC_API MemberNode::~MemberNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(MemberNode);
