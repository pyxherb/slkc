#include "type_base.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API TypeNameNode::TypeNameNode(Global *global, TypeNameKind kind)
	: AstNode(NodeType::TypeName, global),
	  _tn_kind(kind) {
}

SLKC_API TypeNameNode::TypeNameNode(
	const TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: AstNode(other, context, node_index),
	  _tn_kind(other._tn_kind) {
}

SLKC_API TypeNameNode::~TypeNameNode() {
}

SLKC_API DumpResult TypeNameNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(_tn_kind)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("tn_kind", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), static_cast<bool>(_is_const)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("is_const", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), static_cast<bool>(_is_final)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("_is_final", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(_nullability)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("is_nullable", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(_shareability)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("shareability", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	return DumpResult::Ok;
}
