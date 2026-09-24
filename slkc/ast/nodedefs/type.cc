#include "type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I8TypeNameNode);

SLKC_API I8TypeNameNode::I8TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::I8) {
}

SLKC_API I8TypeNameNode::I8TypeNameNode(
	const I8TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API I8TypeNameNode::~I8TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I8TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I16TypeNameNode);

SLKC_API I16TypeNameNode::I16TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::I16) {
}

SLKC_API I16TypeNameNode::I16TypeNameNode(
	const I16TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API I16TypeNameNode::~I16TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I16TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I32TypeNameNode);

SLKC_API I32TypeNameNode::I32TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::I32) {
}

SLKC_API I32TypeNameNode::I32TypeNameNode(
	const I32TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API I32TypeNameNode::~I32TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I32TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(I64TypeNameNode);

SLKC_API I64TypeNameNode::I64TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::I64) {
}

SLKC_API I64TypeNameNode::I64TypeNameNode(
	const I64TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API I64TypeNameNode::~I64TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(I64TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ISizeTypeNameNode);

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::ISize) {
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(
	const ISizeTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API ISizeTypeNameNode::~ISizeTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(ISizeTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U8TypeNameNode);

SLKC_API U8TypeNameNode::U8TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::U8) {
}

SLKC_API U8TypeNameNode::U8TypeNameNode(
	const U8TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API U8TypeNameNode::~U8TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U8TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U16TypeNameNode);

SLKC_API U16TypeNameNode::U16TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::U16) {
}

SLKC_API U16TypeNameNode::U16TypeNameNode(
	const U16TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API U16TypeNameNode::~U16TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U16TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U32TypeNameNode);

SLKC_API U32TypeNameNode::U32TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::U32) {
}

SLKC_API U32TypeNameNode::U32TypeNameNode(
	const U32TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API U32TypeNameNode::~U32TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U32TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(U64TypeNameNode);

SLKC_API U64TypeNameNode::U64TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::U64) {
}

SLKC_API U64TypeNameNode::U64TypeNameNode(
	const U64TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API U64TypeNameNode::~U64TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(U64TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(USizeTypeNameNode);

SLKC_API USizeTypeNameNode::USizeTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::USize) {
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(
	const USizeTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API USizeTypeNameNode::~USizeTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(USizeTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F32TypeNameNode);

SLKC_API F32TypeNameNode::F32TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::F32) {
}

SLKC_API F32TypeNameNode::F32TypeNameNode(
	const F32TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API F32TypeNameNode::~F32TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(F32TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(F64TypeNameNode);

SLKC_API F64TypeNameNode::F64TypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::F64) {
}

SLKC_API F64TypeNameNode::F64TypeNameNode(
	const F64TypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API F64TypeNameNode::~F64TypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(F64TypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StringTypeNameNode);

SLKC_API StringTypeNameNode::StringTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::String) {
}

SLKC_API StringTypeNameNode::StringTypeNameNode(
	const StringTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API StringTypeNameNode::~StringTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(StringTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(BoolTypeNameNode);

SLKC_API BoolTypeNameNode::BoolTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Bool) {
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(
	const BoolTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API BoolTypeNameNode::~BoolTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(BoolTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(VoidTypeNameNode);

SLKC_API VoidTypeNameNode::VoidTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Void) {
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(
	const VoidTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API VoidTypeNameNode::~VoidTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(VoidTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ObjectTypeNameNode);

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Object) {
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(
	const ObjectTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API ObjectTypeNameNode::~ObjectTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(ObjectTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(AnyTypeNameNode);

SLKC_API AnyTypeNameNode::AnyTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Any) {
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(
	const AnyTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API AnyTypeNameNode::~AnyTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(AnyTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(NeverTypeNameNode);

SLKC_API NeverTypeNameNode::NeverTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Never) {
}

SLKC_API NeverTypeNameNode::NeverTypeNameNode(
	const NeverTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index)
	: TypeNameNode(other, context, node_index) {
}

SLKC_API NeverTypeNameNode::~NeverTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(NeverTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(CustomTypeNameNode);

SLKC_API DumpResult CustomTypeNameNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_id_ref(static_cast<wandjson::ArrayValue *>(v.get()), dump_context, referred_name, deep_dump));
	if (!target_object->insert("referred_name", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	return DumpResult::Ok;
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Custom),
	  referred_name(global->get_allocator()) {
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(
	const CustomTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: TypeNameNode(other, context, node_index),
	  referred_name(context.get_global()->get_allocator()) {
	if (auto result = other.referred_name.duplicate(context.get_global()->get_allocator()); result.has_value()) {
		referred_name = std::move(result).value();
	} else {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API CustomTypeNameNode::~CustomTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(CustomTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ArrayTypeNameNode);

SLKC_API DumpResult ArrayTypeNameNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), element_type.get_index(), deep_dump));
	if (!target_object->insert("referred_name", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	return DumpResult::Ok;
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Array) {
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(
	const ArrayTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: TypeNameNode(other, context, node_index) {
	if (auto result = context.push_task(element_type.get_index()); result.is_ok()) {
		element_type = AstNodePtr<TypeNameNode>(context.get_global(), std::move(result).value());
	} else {
		error_out = std::move(result).error();
		return;
	}
}

SLKC_API ArrayTypeNameNode::~ArrayTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ArrayTypeNameNode);

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(RefTypeNameNode);

SLKC_API DumpResult RefTypeNameNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(AstNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;
	wandjson::Value *discarded_v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), element_type.get_index(), deep_dump));
	if (!target_object->insert("referred_name", v.get()))
		return DumpResult::OutOfMemory;
	discarded_v = v.release();

	return DumpResult::Ok;
}

SLKC_API RefTypeNameNode::RefTypeNameNode(Global *global)
	: TypeNameNode(global, TypeNameKind::Ref) {
}

SLKC_API RefTypeNameNode::RefTypeNameNode(
	const RefTypeNameNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: TypeNameNode(other, context, node_index) {
	if (auto result = context.push_task(element_type.get_index()); result.is_ok()) {
		element_type = AstNodePtr<TypeNameNode>(context.get_global(), std::move(result).value());
	} else {
		error_out = std::move(result).error();
		return;
	}
}

SLKC_API RefTypeNameNode::~RefTypeNameNode() {
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(RefTypeNameNode);
