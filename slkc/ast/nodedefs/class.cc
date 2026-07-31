#include "class.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API GenericConstraint::GenericConstraint(Global *global)
	: implemented_types(global->get_allocator()),
	  sti_implement_item_separator(global->get_allocator()) {
}

SLKC_API GenericConstraint::~GenericConstraint() {
}

SLKC_API peff::Result<GenericConstraint, DuplicationError> GenericConstraint::deep_duplicate(DuplicationContext &duplication_context) const noexcept {
	GenericConstraint new_constraint(duplication_context.get_global());

	if (inherited_type.has_value()) {
		auto result = duplication_context.push_task(*inherited_type);
		if (result.has_error())
			return std::move(result).error();
		new_constraint.inherited_type = std::move(result).value();
	}

	{
		if (!new_constraint.implemented_types.resize(this->implemented_types.size()))
			return DuplicationError::OutOfMemory;

		const size_t limit = implemented_types.size();
		for (size_t i = 0; i < limit; ++i) {
			new_constraint.implemented_types[i] = implemented_types[i];

			auto result = duplication_context.push_task(implemented_types[i].type);
			if (!result.has_error())
				return std::move(result).error();
			new_constraint.implemented_types[i].type = std::move(result).value();
		}
	}

	new_constraint.generic_variance = generic_variance;

	new_constraint.sti_inherit_left_parenthesis = sti_inherit_left_parenthesis;
	new_constraint.sti_inherit_right_parenthesis = sti_inherit_right_parenthesis;
	new_constraint.sti_implement_colon = sti_implement_colon;
	new_constraint.sti_generic_variance_indicator = sti_generic_variance_indicator;

	if (!new_constraint.sti_implement_item_separator.build(sti_implement_item_separator))
		return DuplicationError::OutOfMemory;

	return std::move(new_constraint);
}

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(GenericParamNode);

SLKC_API DumpResult GenericParamNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.release()))
		return DumpResult::OutOfMemory;

	// TODO: Dump the generic constraint.

	return DumpResult::Ok;
}

SLKC_API GenericParamNode::GenericParamNode(Global *global)
	: MemberNode(NodeType::GenericParam, global) {
}

SLKC_API GenericParamNode::GenericParamNode(
	const GenericParamNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_name(other.sti_name) {
	if (error_out.has_value())
		return;

	{
		auto result = other.generic_constraint->deep_duplicate(context);
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		generic_constraint = std::move(result).value();
	}
}

SLKC_API GenericParamNode::~GenericParamNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(GenericParamNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ClassNode);

SLKC_API DumpResult ClassNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_class_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_class_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ClassNode::ClassNode(Global *global)
	: MemberNode(NodeType::Class, global) {
}

SLKC_API ClassNode::ClassNode(
	const ClassNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_class_keyword(other.sti_class_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API ClassNode::~ClassNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ClassNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(InterfaceNode);

SLKC_API DumpResult InterfaceNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_interface_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_interface_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API InterfaceNode::InterfaceNode(Global *global)
	: MemberNode(NodeType::Interface, global) {
}

SLKC_API InterfaceNode::InterfaceNode(
	const InterfaceNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_interface_keyword(other.sti_interface_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API InterfaceNode::~InterfaceNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(InterfaceNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ExceptNode);

SLKC_API DumpResult ExceptNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_except_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_except_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ExceptNode::ExceptNode(Global *global)
	: MemberNode(NodeType::Except, global) {
}

SLKC_API ExceptNode::ExceptNode(
	const ExceptNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_except_keyword(other.sti_except_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API ExceptNode::~ExceptNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ExceptNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(TraitNode);

SLKC_API DumpResult TraitNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_trait_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_trait_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API TraitNode::TraitNode(Global *global)
	: MemberNode(NodeType::Trait, global) {
}

SLKC_API TraitNode::TraitNode(
	const TraitNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_trait_keyword(other.sti_trait_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API TraitNode::~TraitNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(TraitNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(StructNode);

SLKC_API DumpResult StructNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_struct_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_struct_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API StructNode::StructNode(Global *global)
	: MemberNode(NodeType::Struct, global) {
}

SLKC_API StructNode::StructNode(
	const StructNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_struct_keyword(other.sti_struct_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API StructNode::~StructNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StructNode);

SLKC_API DumpResult ConstEnumNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_const_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_const_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ConstEnumNode::ConstEnumNode(Global *global)
	: MemberNode(NodeType::ConstEnum, global) {
}

SLKC_API ConstEnumNode::ConstEnumNode(
	const ConstEnumNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_enum_keyword(other.sti_enum_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API ConstEnumNode::~ConstEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ConstEnumNode);

SLKC_API DumpResult ScopedEnumNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API ScopedEnumNode::ScopedEnumNode(Global *global)
	: MemberNode(NodeType::ScopedEnum, global) {
}

SLKC_API ScopedEnumNode::ScopedEnumNode(
	const ScopedEnumNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_enum_keyword(other.sti_enum_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API ScopedEnumNode::~ScopedEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ScopedEnumNode);

SLKC_API DumpResult UnionEnumNode::do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_union_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_union_keyword", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API UnionEnumNode::UnionEnumNode(Global *global)
	: MemberNode(NodeType::UnionEnum, global) {
}

SLKC_API UnionEnumNode::UnionEnumNode(
	const UnionEnumNode &other,
	DuplicationContext &context,
	NodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_enum_keyword(other.sti_enum_keyword) {
	if (error_out.has_value())
		return;
}

SLKC_API UnionEnumNode::~UnionEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(UnionEnumNode);
