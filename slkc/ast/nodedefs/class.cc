#include "class.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API GenericConstraint::GenericConstraint(Global *global)
	: implemented_types(global->get_allocator()),
	  sti_implement_item_separator(global->get_allocator()) {
}

SLKC_API GenericConstraint::~GenericConstraint() {
}

SLKC_API peff::Result<GenericConstraint, DuplicationError> GenericConstraint::deep_duplicate(AstNodeDuplicationContext &duplication_context) const noexcept {
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

SLKC_API DumpResult GenericParamNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	// TODO: Dump the generic constraint.

	return DumpResult::Ok;
}

SLKC_API GenericParamNode::GenericParamNode(Global *global)
	: MemberNode(NodeType::GenericParam, global),
	  generic_constraint(global) {
}

SLKC_API GenericParamNode::GenericParamNode(
	const GenericParamNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  generic_constraint(context.get_global()),
	  sti_name(other.sti_name) {
	if (error_out.has_value())
		return;

	{
		auto result = other.generic_constraint.deep_duplicate(context);
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

SLKC_API DumpResult ClassNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_class_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_class_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_inherit_left_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_inherit_left_parenthesis", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_inherit_right_parenthesis))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_inherit_right_parenthesis", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_implement_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_implement_colon", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_implement_item_separator", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_implement_item_separator) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_generic_params_comma_separators", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_generic_params_comma_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API ClassNode::ClassNode(Global *global)
	: MemberNode(NodeType::Class, global),
	  sti_implement_item_separator(global->get_allocator()),
	  sti_generic_params_comma_separators(global->get_allocator()) {
}

SLKC_API ClassNode::ClassNode(
	const ClassNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_class_keyword(other.sti_class_keyword),
	  sti_name(other.sti_name),
	  sti_generic_left_angle(other.sti_generic_left_angle),
	  sti_generic_right_angle(other.sti_generic_right_angle),
	  sti_inherit_left_parenthesis(other.sti_inherit_left_parenthesis),
	  sti_inherit_right_parenthesis(other.sti_inherit_right_parenthesis),
	  sti_implement_colon(other.sti_implement_colon),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace),
	  sti_implement_item_separator(context.get_global()->get_allocator()),
	  sti_generic_params_comma_separators(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;
	if (!sti_implement_item_separator.build(other.sti_implement_item_separator)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	if (!sti_generic_params_comma_separators.build(other.sti_generic_params_comma_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API ClassNode::~ClassNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ClassNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(InterfaceNode);

SLKC_API DumpResult InterfaceNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_interface_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_interface_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_implement_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_implement_colon", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_implement_item_separator", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_implement_item_separator) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_generic_params_comma_separators", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_generic_params_comma_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API InterfaceNode::InterfaceNode(Global *global)
	: MemberNode(NodeType::Interface, global),
	  sti_implement_item_separator(global->get_allocator()),
	  sti_generic_params_comma_separators(global->get_allocator()) {
}

SLKC_API InterfaceNode::InterfaceNode(
	const InterfaceNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_interface_keyword(other.sti_interface_keyword),
	  sti_name(other.sti_name),
	  sti_implement_colon(other.sti_implement_colon),
	  sti_implement_item_separator(context.get_global()->get_allocator()),
	  sti_generic_params_comma_separators(context.get_global()->get_allocator()),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
	if (!sti_implement_item_separator.build(other.sti_implement_item_separator)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	if (!sti_generic_params_comma_separators.build(other.sti_generic_params_comma_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API InterfaceNode::~InterfaceNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(InterfaceNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ExceptNode);

SLKC_API DumpResult ExceptNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_except_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_except_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API ExceptNode::ExceptNode(Global *global)
	: MemberNode(NodeType::Except, global) {
}

SLKC_API ExceptNode::ExceptNode(
	const ExceptNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_except_keyword(other.sti_except_keyword),
	  sti_name(other.sti_name),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API ExceptNode::~ExceptNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ExceptNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(TraitNode);

SLKC_API DumpResult TraitNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_trait_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_trait_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API TraitNode::TraitNode(Global *global)
	: MemberNode(NodeType::Trait, global) {
}

SLKC_API TraitNode::TraitNode(
	const TraitNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_trait_keyword(other.sti_trait_keyword),
	  sti_name(other.sti_name),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API TraitNode::~TraitNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(TraitNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(StructNode);

SLKC_API DumpResult StructNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_struct_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_struct_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_generic_left_angle))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_generic_left_angle", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_generic_right_angle))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_generic_right_angle", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_implement_colon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_implement_colon", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_implement_item_separator", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_implement_item_separator) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	{
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *av = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("sti_generic_params_comma_separators", v.get()))
			return DumpResult::OutOfMemory;
		v.release();

		for (const auto i : sti_generic_params_comma_separators) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), i))))
				return DumpResult::OutOfMemory;
			if (!av->push_back(v.get()))
				return DumpResult::OutOfMemory;
			v.release();
		}
	}

	return DumpResult::Ok;
}

SLKC_API StructNode::StructNode(Global *global)
	: MemberNode(NodeType::Struct, global),
	  sti_implement_item_separator(global->get_allocator()),
	  sti_generic_params_comma_separators(global->get_allocator()) {
}

SLKC_API StructNode::StructNode(
	const StructNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_struct_keyword(other.sti_struct_keyword),
	  sti_implement_colon(other.sti_implement_colon),
	  sti_name(other.sti_name),
	  sti_generic_left_angle(other.sti_generic_left_angle),
	  sti_generic_right_angle(other.sti_generic_right_angle),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace),
	  sti_implement_item_separator(context.get_global()->get_allocator()),
	  sti_generic_params_comma_separators(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;
	if (!sti_implement_item_separator.build(other.sti_implement_item_separator)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
	if (!sti_generic_params_comma_separators.build(other.sti_generic_params_comma_separators)) {
		error_out = DuplicationError::OutOfMemory;
		return;
	}
}

SLKC_API StructNode::~StructNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(StructNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ConstEnumNode);

SLKC_API DumpResult ConstEnumNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_const_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_const_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API ConstEnumNode::ConstEnumNode(Global *global)
	: MemberNode(NodeType::ConstEnum, global) {
}

SLKC_API ConstEnumNode::ConstEnumNode(
	const ConstEnumNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_const_keyword(other.sti_const_keyword),
	  sti_name(other.sti_name),
	  sti_enum_keyword(other.sti_enum_keyword),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API ConstEnumNode::~ConstEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ConstEnumNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ScopedEnumNode);

SLKC_API DumpResult ScopedEnumNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API ScopedEnumNode::ScopedEnumNode(Global *global)
	: MemberNode(NodeType::ScopedEnum, global) {
}

SLKC_API ScopedEnumNode::ScopedEnumNode(
	const ScopedEnumNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_enum_keyword(other.sti_enum_keyword),
	  sti_name(other.sti_name),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API ScopedEnumNode::~ScopedEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ScopedEnumNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(EnumItemNode);

SLKC_API DumpResult EnumItemNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (specified_value) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), specified_value.get_index(), deep_dump));
		if (!target_object->insert("specified_value", v.get()))
			return DumpResult::OutOfMemory;
		v.release();
	}

	return DumpResult::Ok;
}

SLKC_API EnumItemNode::EnumItemNode(Global *global)
	: MemberNode(NodeType::EnumItem, global) {
}

SLKC_API EnumItemNode::EnumItemNode(
	const EnumItemNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out) {
	if (error_out.has_value())
		return;

	{
		auto result = context.push_task(other.specified_value.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		specified_value = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API EnumItemNode::~EnumItemNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(EnumItemNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(UnionEnumNode);

SLKC_API DumpResult UnionEnumNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_enum_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_enum_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_union_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_union_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API UnionEnumNode::UnionEnumNode(Global *global)
	: MemberNode(NodeType::UnionEnum, global) {
}

SLKC_API UnionEnumNode::UnionEnumNode(
	const UnionEnumNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_enum_keyword(other.sti_enum_keyword),
	  sti_union_keyword(other.sti_union_keyword),
	  sti_name(other.sti_name),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API UnionEnumNode::~UnionEnumNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(UnionEnumNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(UnionEnumItemNode);

SLKC_API DumpResult UnionEnumItemNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	return DumpResult::Ok;
}

SLKC_API UnionEnumItemNode::UnionEnumItemNode(Global *global)
	: MemberNode(NodeType::UnionEnumItem, global) {
}

SLKC_API UnionEnumItemNode::UnionEnumItemNode(
	const UnionEnumItemNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out) {
	if (error_out.has_value())
		return;
}

SLKC_API UnionEnumItemNode::~UnionEnumItemNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(UnionEnumItemNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(AttributeNode);

SLKC_API DumpResult AttributeNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_attribute_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_attribute_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_name))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_name", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_left_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_left_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_right_brace))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_right_brace", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API AttributeNode::AttributeNode(Global *global)
	: MemberNode(NodeType::Attribute, global) {
}

SLKC_API AttributeNode::AttributeNode(
	const AttributeNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_attribute_keyword(other.sti_attribute_keyword),
	  sti_name(other.sti_name),
	  sti_left_brace(other.sti_left_brace),
	  sti_right_brace(other.sti_right_brace) {
	if (error_out.has_value())
		return;
}

SLKC_API AttributeNode::~AttributeNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(AttributeNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(ImportNode);

SLKC_API DumpResult ImportNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_import_keyword))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_import_keyword", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), sti_semicolon))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("sti_semicolon", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API ImportNode::ImportNode(Global *global)
	: MemberNode(NodeType::Import, global),
	  id_ref(global->get_allocator()) {
}

SLKC_API ImportNode::ImportNode(
	const ImportNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  sti_import_keyword(other.sti_import_keyword),
	  sti_semicolon(other.sti_semicolon),
	  id_ref(context.get_global()->get_allocator()) {
	if (error_out.has_value())
		return;
	if (auto result = other.id_ref.duplicate(other.get_global()->get_allocator()); !result.has_value()) {
		error_out = DuplicationError::OutOfMemory;
		return;
	} else {
		id_ref = std::move(result).value();
	}
}

SLKC_API ImportNode::~ImportNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(ImportNode);

SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(VarNode);

SLKC_API DumpResult VarNode::do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept {
	SLKC_RETURN_IF_DUMP_FAILED(MemberNode::do_dump(dump_context, target_object, deep_dump));

	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), init_value.get_index(), deep_dump));
	if (!target_object->insert("init_value", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	if (!(v = decltype(v)(wandjson::BooleanValue::alloc(dump_context.get_allocator(), is_var_binding))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("is_var_binding", v.get()))
		return DumpResult::OutOfMemory;
	v.release();

	return DumpResult::Ok;
}

SLKC_API VarNode::VarNode(Global *global)
	: MemberNode(NodeType::EnumItem, global) {
}

SLKC_API VarNode::VarNode(
	const VarNode &other,
	AstNodeDuplicationContext &context,
	AstNodeIndex node_index,
	peff::Option<DuplicationError> &error_out)
	: MemberNode(other, context, node_index, error_out),
	  is_var_binding(other.is_var_binding) {
	if (error_out.has_value())
		return;

	{
		auto result = context.push_task(other.init_value.get_index());
		if (result.has_error()) {
			error_out = std::move(result).error();
			return;
		}
		init_value = AstNodePtr<ExprNode>(context.get_global(), std::move(result).value());
	}
}

SLKC_API VarNode::~VarNode() {
}

SLKC_SIMPLE_AST_DEALLOC_FN_DEF(VarNode);
