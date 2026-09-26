#define NOMINMAX
#include "rg2ast.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API peff::Option<CompilationError> comp::_pin_fail_reason_to_comp_error(PinFailReason reason) {
	switch (reason) {
		case PinFailReason::IOError:
			return gen_pinning_io_error_option();
		case PinFailReason::OutOfMemory:
			return gen_oom_error_option();
		case PinFailReason::OutOfNodeIndex:
			return gen_out_of_node_index_error_option();
		default:
			std::terminate();
	}
}

SLKC_API peff::Option<CompilationError> comp::_green_node_op_result_to_comp_error(ast::GreenNodeOperationResult result) {
	switch (result) {
		case ast::GreenNodeOperationResult::Success:
			return peff::NULLOPT;
		case ast::GreenNodeOperationResult::PinIOError:
			return gen_pinning_io_error_option();
		case ast::GreenNodeOperationResult::OutOfMemory:
			return gen_oom_error_option();
		case ast::GreenNodeOperationResult::OutOfNodeIndex:
			return gen_out_of_node_index_error_option();
	}
	std::terminate();
}

template <typename T>
SLAKE_FORCEINLINE static peff::Option<CompilationError> _parse_int(
	CompilationEnv *env,
	const ast::TokenPtr &token,
	bool is_negative,
	const std::string_view &body_view,
	T &data_out) {
	peff::Option<CompilationError> syntax_error;

	bool overflow_warned = false;
	char c;
	T data = 0;
	size_t i = (size_t)is_negative;

	auto push_overflowed_error = [env, &token]() noexcept -> peff::Option<CompilationError> {
		SLKC_RETURN_IF_COMP_ERROR(
			env->push_error(
				CompilationError(
					ast::TokenRange{ env->get_target_module().get_index(), token->index }, CompilationErrorKind::LiteralOverflowed)));
		return peff::NULLOPT;
	};

	switch (((ast::IntTokenExtension *)token->ex_data.get())->token_type) {
		case ast::IntTokenType::Decimal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 10 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 10;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 10;
					data += c - '0';
				}
				++i;
			}
			break;
		case ast::IntTokenType::Hexadecimal:
			while (i < body_view.size()) {
				char c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 16 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data -= c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data -= c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data -= c - 'A' + 10;
							break;
					}
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data += c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data += c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data += c - 'A' + 10;
							break;
					}
				}
				++i;
			}
			break;
		case ast::IntTokenType::Octal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 8 > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 8;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 8 < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data *= 8;
					data += c - '0';
				}
				++i;
			}
			break;
		case ast::IntTokenType::Binary:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && ((std::numeric_limits<T>::min() >> 1) > data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data <<= 1;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && ((std::numeric_limits<T>::max() >> 1) < data)) {
						SLKC_RETURN_IF_COMP_ERROR(push_overflowed_error());
						overflow_warned = true;
					}
					data <<= 1;
					data += c - '0';
				}
				++i;
			}
			break;
		default:
			std::terminate();
	}

	data_out = data;

	return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_var_binding(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::BindingEntry &binding_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	auto g = red_node->as_green_node();
	assert(g->node_kind == ast::GreenNodeKind::VarBinding);

	if (indices.get_classified_indices(ast::TokenId::VarKeyword).size())
		binding_out.is_var_binding = true;

	{
		ast::RedNodePtr name_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

		binding_out.name = name_node->as_token()->source_text;
	}

	if (auto index = indices.get_classified_indices(ast::GreenNodeKind::TypeName); index.size()) {
		ast::RedNodePtr tn_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->get_child_node(state_allocator, index[0], tn_node)));

		ast::AstNodePin<ast::TypeNameNode> tn;
		SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, tn_node, tn)(sched));
		binding_out.type = tn;
	}

	if (auto index = indices.get_classified_indices(ast::GreenNodeKind::Expr); index.size()) {
		ast::RedNodePtr init_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->get_child_node(state_allocator, index[0], init_node)));

		ast::AstNodePin<ast::AstNode> init_expr;

		SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, init_node, init_expr)(sched));

		binding_out.initial_value = std::move(init_expr).cast_to<ast::ExprNode>();
	}

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::AstNodePin<ast::TypeNameNode> &type_name_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	auto g = red_node->as_green_node();
	assert(g->node_kind == ast::GreenNodeKind::TypeName);

	auto exdata = std::get<ast::TypeNameGreenNodeExData>(g->exdata);

	switch (exdata.type_name_kind) {
		case ast::GreenNodeTypeNameKind::I8TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::I8TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::I16TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::I16TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::I32TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::I32TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::I64TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::I64TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::ISizeTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::ISizeTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::U8TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::U8TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::U16TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::U16TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::U32TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::U32TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::U64TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::U64TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::USizeTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::USizeTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::F32TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::F32TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::F64TypeName:
			if (!(type_name_out = ast::make_ast_node<ast::F64TypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::StringTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::StringTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::BoolTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::BoolTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::VoidTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::VoidTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::ObjectTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::ObjectTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::AnyTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::AnyTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::NeverTypeName:
			if (!(type_name_out = ast::make_ast_node<ast::NeverTypeNameNode>(env->get_global()).cast_to<ast::TypeNameNode>()))
				co_return gen_oom_error_option();
			break;
		case ast::GreenNodeTypeNameKind::CustomTypeName: {
			auto tn = ast::make_ast_node<ast::CustomTypeNameNode>(env->get_global());

			if (!tn)
				co_return gen_oom_error_option();
			type_name_out = tn.cast_to<ast::TypeNameNode>();

			ast::RedNodePtr id_ref_node;
			SLKC_CO_RETURN_IF_COMP_ERROR(
				_green_node_op_result_to_comp_error(
					red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::IdRef)[0], id_ref_node)));

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_id_ref(state_allocator, sched, env, id_ref_node, tn->referred_name)(sched));
			break;
		}
		case ast::GreenNodeTypeNameKind::ArrayTypeName: {
			auto tn = ast::make_ast_node<ast::ArrayTypeNameNode>(env->get_global());

			if (!tn)
				co_return gen_oom_error_option();
			type_name_out = tn.cast_to<ast::TypeNameNode>();

			ast::RedNodePtr element_node;
			SLKC_CO_RETURN_IF_COMP_ERROR(
				_green_node_op_result_to_comp_error(
					red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], element_node)));

			ast::AstNodePin<ast::TypeNameNode> tn_pin;
			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, element_node, tn_pin)(sched));

			tn->element_type = tn_pin;
			break;
		}
		default:
			std::terminate();
	}

	// Apply the modifiers collected by the parser.
	if (indices.get_classified_indices(ast::TokenId::ConstKeyword).size())
		type_name_out->set_const(true);
	if (indices.get_classified_indices(ast::TokenId::FinalKeyword).size())
		type_name_out->set_final(true);
	if (indices.get_classified_indices(ast::TokenId::Question).size())
		type_name_out->set_nullability(ast::TypeNameNullability::Nullable);
	else if (indices.get_classified_indices(ast::TokenId::LNotOp).size())
		type_name_out->set_nullability(ast::TypeNameNullability::NonNullable);
	/* if (indices.get_classified_indices(ast::TokenId::RefKeyword).size())
		type_name_out.set_ref(true);
	if (indices.get_classified_indices(ast::TokenId::ReadonlyKeyword).size())
		type_name_out.set_readonly_ref(true);
	if (indices.get_classified_indices(ast::TokenId::RestrictKeyword).size())
		type_name_out.set_shareability(ast::TypeNameShareability::Restrict);
	else if (indices.get_classified_indices(ast::TokenId::MultiKeyword).size())
		type_name_out.set_shareability(ast::TypeNameShareability::Multi);
	else if (indices.get_classified_indices(ast::TokenId::SynchronizedKeyword).size())
		type_name_out.set_shareability(ast::TypeNameShareability::Synchronized);*/

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_id_ref(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::OwnedIdRef &id_ref_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	auto g = red_node->as_green_node();
	assert(g->node_kind == ast::GreenNodeKind::IdRef);

	for (auto i : indices.get_classified_indices(ast::GreenNodeKind::IdRefEntry)) {
		ast::RedNodePtr entry_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				red_node->get_child_node(state_allocator, i, entry_node)));

		assert(entry_node->is_green_node_facade());

		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				entry_node->build_children(env->get_global()->get_allocator())));

		ast::IdRefEntry ast_entry(env->get_global()->get_allocator());

		ast::RedNodeChildIndices entry_indices(state_allocator);

		if (!entry_indices.index_children(entry_node))
			co_return gen_oom_error_option();

		ast::RedNodePtr id_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				entry_node->get_child_node(state_allocator, entry_indices.get_classified_indices(ast::TokenId::Id)[0], id_node)));

		ast_entry.name = id_node->as_token()->source_text;

		for (auto j : entry_indices.get_classified_indices(ast::GreenNodeKind::TypeName)) {
			ast::RedNodePtr tn_node;
			SLKC_CO_RETURN_IF_COMP_ERROR(
				_green_node_op_result_to_comp_error(
					entry_node->get_child_node(state_allocator, j, tn_node)));

			ast::AstNodePin<ast::TypeNameNode> tn;

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, tn_node, tn)(sched));

			if (!ast_entry.generic_args.push_back(std::move(tn)))
				co_return gen_oom_error_option();
		}

		if (!id_ref_out.entries.push_back(std::move(ast_entry)))
			co_return gen_oom_error_option();
	}

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_nodes_to_ast_members(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, const ast::AstNodePin<ast::MemberNode> &node_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	for (auto i : red_node->children) {
		if (i->is_green_node_facade()) {
			auto node = i->as_green_node();

			switch (node->node_kind) {
				case ast::GreenNodeKind::FnDecl:
				case ast::GreenNodeKind::FnDef:
				case ast::GreenNodeKind::ClassDef:
				case ast::GreenNodeKind::InterfaceDef:
				case ast::GreenNodeKind::TraitDef:
				case ast::GreenNodeKind::StructDef:
				case ast::GreenNodeKind::ConstEnumDef:
				case ast::GreenNodeKind::ScopedEnumDef:
				case ast::GreenNodeKind::UnionEnumDef:
				case ast::GreenNodeKind::ExceptDef:
				case ast::GreenNodeKind::AttributeDef:
				case ast::GreenNodeKind::ConstAndScopedEnumItem:
				case ast::GreenNodeKind::UnionEnumCase: {
					ast::AstNodePin<ast::AstNode> ast_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, i, ast_node)(sched));

					if (node_out->get_scope()->push_member(ast_node.cast_to<ast::MemberNode>()) == SIZE_MAX)
						co_return gen_oom_error_option();

					ast_node.cast_to<ast::MemberNode>()->set_parent(node_out.get_index());
					break;
				}
				case ast::GreenNodeKind::Var: {
					bool is_var_binding = indices.get_classified_indices(ast::TokenId::VarKeyword).size();

					auto var_bindings_indices = indices.get_classified_indices(ast::GreenNodeKind::VarBindings);

					if (var_bindings_indices.empty())
						co_return peff::NULLOPT;

					ast::RedNodePtr var_bindings_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->get_child_node(state_allocator, var_bindings_indices[0], var_bindings_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(var_bindings_node->build_children(state_allocator)));

					ast::RedNodeChildIndices bindings_indices(state_allocator);

					if (!bindings_indices.index_children(var_bindings_node))
						co_return gen_oom_error_option();

					auto binding_indices = bindings_indices.get_classified_indices(ast::GreenNodeKind::VarBinding);

					for (auto i : binding_indices) {
						ast::RedNodePtr binding_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(var_bindings_node->get_child_node(state_allocator, i, binding_node)));

						ast::AstNodePin<ast::VarNode> var_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _lower_rg_var_binding_to_var_node(state_allocator, sched, env, binding_node, is_var_binding, var_node)(sched));

						if (node_out->get_scope()->push_member(var_node.cast_to<ast::MemberNode>()) == SIZE_MAX)
							co_return gen_oom_error_option();

						var_node->set_parent(node_out.get_index());
						break;
					}
					break;
				}
			}
		}
	}

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_lower_rg_impl_list_to_scope(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &impl_list_node, ast::Scope *scope_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(impl_list_node->build_children(state_allocator)));

	ast::RedNodeChildIndices list_indices(state_allocator);

	if (!list_indices.index_children(impl_list_node))
		co_return gen_oom_error_option();

	for (auto i : list_indices.get_classified_indices(ast::GreenNodeKind::ImplItem)) {
		ast::RedNodePtr item_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				impl_list_node->get_child_node(state_allocator, i, item_node)));

		SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(item_node->build_children(state_allocator)));

		ast::RedNodeChildIndices item_indices(state_allocator);

		if (!item_indices.index_children(item_node))
			co_return gen_oom_error_option();

		ast::ImplementItem item;

		if (item_indices.get_classified_indices(ast::TokenId::TraitKeyword).size())
			item.is_trait = true;

		ast::RedNodePtr item_tn_node;
		SLKC_CO_RETURN_IF_COMP_ERROR(
			_green_node_op_result_to_comp_error(
				item_node->get_child_node(state_allocator, item_indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], item_tn_node)));

		ast::AstNodePin<ast::TypeNameNode> tn;
		SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, item_tn_node, tn)(sched));
		item.type = tn;

		if (!scope_out->implemented_types.push_back(std::move(item)))
			co_return gen_oom_error_option();
	}

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_lower_rg_inheritance_slot_to_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &inheritance_slot_node, ast::AstNodePtr<ast::TypeNameNode> &type_name_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(inheritance_slot_node->build_children(state_allocator)));

	ast::RedNodeChildIndices slot_indices(state_allocator);

	if (!slot_indices.index_children(inheritance_slot_node))
		co_return gen_oom_error_option();

	ast::RedNodePtr tn_node;
	SLKC_CO_RETURN_IF_COMP_ERROR(
		_green_node_op_result_to_comp_error(
			inheritance_slot_node->get_child_node(state_allocator, slot_indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], tn_node)));

	ast::AstNodePin<ast::TypeNameNode> tn;
	SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, tn_node, tn)(sched));
	type_name_out = tn;

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_lower_rg_var_binding_to_var_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &binding_node, bool is_var_binding, ast::AstNodePin<ast::VarNode> &var_node_out) {
	ast::BindingEntry binding;

	SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_var_binding(state_allocator, sched, env, binding_node, binding)(sched));

	ast::AstNodePin<ast::VarNode> var_node = ast::make_ast_node<ast::VarNode>(env->get_global());

	if (!var_node)
		co_return _pin_fail_reason_to_comp_error(var_node.get_fail_reason());

	var_node->set_name(binding.name);
	var_node->is_var_binding = is_var_binding;
	var_node->init_value = std::move(binding.initial_value);

	var_node_out = std::move(var_node);

	co_return peff::NULLOPT;
}

SLKC_API CompilationCoroutine comp::_do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::AstNodePin<ast::AstNode> &ast_node_out) {
	SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(red_node->build_children(state_allocator)));

	assert(red_node->is_green_node_facade());

	ast::RedNodeChildIndices indices(state_allocator);

	if (!indices.index_children(red_node))
		co_return gen_oom_error_option();

	// TODO: Implement it.
	auto g = red_node->as_green_node();
	switch (g->node_kind) {
		case ast::GreenNodeKind::Expr: {
			auto exdata = std::get<ast::ExprGreenNodeExData>(g->exdata);

			// TODO: Give anchors of tokens to the AST nodes.
			switch (exdata.expr_kind) {
				case slkc::ast::GreenNodeExprKind::I8Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I8LiteralExprNode> e = ast::make_ast_node<ast::I8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I16Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I16Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I16LiteralExprNode> e = ast::make_ast_node<ast::I16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I32Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I32LiteralExprNode> e = ast::make_ast_node<ast::I32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::I64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::I64Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					int64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::I64LiteralExprNode> e = ast::make_ast_node<ast::I64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U8Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U8Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint8_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U8LiteralExprNode> e = ast::make_ast_node<ast::U8LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U16Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U16Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint16_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U16LiteralExprNode> e = ast::make_ast_node<ast::U16LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U32Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint32_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U32LiteralExprNode> e = ast::make_ast_node<ast::U32LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::U64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::U64Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					uint64_t literal = 0;
					bool is_negative = t->source_text.get_view()[0] == '-';
					SLKC_CO_RETURN_IF_COMP_ERROR(_parse_int(env, literal_node->as_token(), is_negative, t->source_text.get_view(), literal));

					ast::AstNodePin<ast::U64LiteralExprNode> e = ast::make_ast_node<ast::U64LiteralExprNode>(env->get_global(), literal);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F32Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::F32Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					float literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F32LiteralExprNode> e = ast::make_ast_node<ast::F32LiteralExprNode>(env->get_global(), strtof(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::F64Literal: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::F64Literal)[0], literal_node)));

					auto t = literal_node->as_token();

					double literal = 0;

					auto view = t->source_text.get_view();
					const char *end_ptr = (&view.back()) + 1;
					ast::AstNodePin<ast::F64LiteralExprNode> e = ast::make_ast_node<ast::F64LiteralExprNode>(env->get_global(), strtod(view.data(), const_cast<char **>(&end_ptr)));

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::StringLiteral: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::StringLiteral)[0], literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::StringLiteralExprNode> e = ast::make_ast_node<ast::StringLiteralExprNode>(env->get_global(), static_cast<ast::StringTokenExtension *>(t->ex_data.get())->data);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::BoolLiteral: {
					auto true_index = indices.get_classified_indices(ast::TokenId::TrueKeyword);
					auto false_index = indices.get_classified_indices(ast::TokenId::FalseKeyword);

					size_t literal_child_index = true_index.size() ? true_index[0] : false_index[0];

					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, literal_child_index, literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::BoolLiteralExprNode> e = ast::make_ast_node<ast::BoolLiteralExprNode>(env->get_global(), t->token_id == ast::TokenId::TrueKeyword);

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::NullLiteral: {
					ast::RedNodePtr literal_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::NullKeyword)[0], literal_node)));

					auto t = literal_node->as_token();

					ast::AstNodePin<ast::NullLiteralExprNode> e = ast::make_ast_node<ast::NullLiteralExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Unary: {
					ast::UnaryOp op;

					switch (exdata.unary_expr_op) {
						case ast::GreenNodeUnaryExprOp::LNot:
							op = ast::UnaryOp::LNot;
							break;
						case ast::GreenNodeUnaryExprOp::Not:
							op = ast::UnaryOp::Not;
							break;
						case ast::GreenNodeUnaryExprOp::Neg:
							op = ast::UnaryOp::Neg;
							break;
						case ast::GreenNodeUnaryExprOp::Move:
							op = ast::UnaryOp::Move;
							break;
						case ast::GreenNodeUnaryExprOp::Unpacking:
							op = ast::UnaryOp::Unpacking;
							break;
						default:
							std::terminate();
					}

					ast::AstNodePin<ast::UnaryExprNode> e = ast::make_ast_node<ast::UnaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					e->unary_op = op;

					ast::RedNodePtr operand_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], operand_node)));

					ast::AstNodePin<ast::AstNode> operand;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, operand_node, operand)(sched));

					e->operand = operand.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::Binary: {
					ast::BinaryOp op;

					switch (exdata.binary_expr_op) {
						case ast::GreenNodeBinaryExprOp::Add:
							op = ast::BinaryOp::Add;
							break;
						case ast::GreenNodeBinaryExprOp::Sub:
							op = ast::BinaryOp::Sub;
							break;
						case ast::GreenNodeBinaryExprOp::Mul:
							op = ast::BinaryOp::Mul;
							break;
						case ast::GreenNodeBinaryExprOp::Div:
							op = ast::BinaryOp::Div;
							break;
						case ast::GreenNodeBinaryExprOp::Mod:
							op = ast::BinaryOp::Mod;
							break;
						case ast::GreenNodeBinaryExprOp::And:
							op = ast::BinaryOp::And;
							break;
						case ast::GreenNodeBinaryExprOp::Or:
							op = ast::BinaryOp::Or;
							break;
						case ast::GreenNodeBinaryExprOp::Xor:
							op = ast::BinaryOp::Xor;
							break;
						case ast::GreenNodeBinaryExprOp::LAnd:
							op = ast::BinaryOp::LAnd;
							break;
						case ast::GreenNodeBinaryExprOp::LOr:
							op = ast::BinaryOp::LOr;
							break;
						case ast::GreenNodeBinaryExprOp::Shl:
							op = ast::BinaryOp::Shl;
							break;
						case ast::GreenNodeBinaryExprOp::Shr:
							op = ast::BinaryOp::Shr;
							break;
						case ast::GreenNodeBinaryExprOp::Assign:
							op = ast::BinaryOp::Assign;
							break;
						case ast::GreenNodeBinaryExprOp::AddAssign:
							op = ast::BinaryOp::AddAssign;
							break;
						case ast::GreenNodeBinaryExprOp::SubAssign:
							op = ast::BinaryOp::SubAssign;
							break;
						case ast::GreenNodeBinaryExprOp::MulAssign:
							op = ast::BinaryOp::MulAssign;
							break;
						case ast::GreenNodeBinaryExprOp::DivAssign:
							op = ast::BinaryOp::DivAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ModAssign:
							op = ast::BinaryOp::ModAssign;
							break;
						case ast::GreenNodeBinaryExprOp::AndAssign:
							op = ast::BinaryOp::AndAssign;
							break;
						case ast::GreenNodeBinaryExprOp::OrAssign:
							op = ast::BinaryOp::OrAssign;
							break;
						case ast::GreenNodeBinaryExprOp::XorAssign:
							op = ast::BinaryOp::XorAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ShlAssign:
							op = ast::BinaryOp::ShlAssign;
							break;
						case ast::GreenNodeBinaryExprOp::ShrAssign:
							op = ast::BinaryOp::ShrAssign;
							break;
						case ast::GreenNodeBinaryExprOp::Eq:
							op = ast::BinaryOp::Eq;
							break;
						case ast::GreenNodeBinaryExprOp::Neq:
							op = ast::BinaryOp::Neq;
							break;
						case ast::GreenNodeBinaryExprOp::PhyEq:
							op = ast::BinaryOp::PhyEq;
							break;
						case ast::GreenNodeBinaryExprOp::PhyNeq:
							op = ast::BinaryOp::PhyNeq;
							break;
						case ast::GreenNodeBinaryExprOp::Lt:
							op = ast::BinaryOp::Lt;
							break;
						case ast::GreenNodeBinaryExprOp::Gt:
							op = ast::BinaryOp::Gt;
							break;
						case ast::GreenNodeBinaryExprOp::LtEq:
							op = ast::BinaryOp::LtEq;
							break;
						case ast::GreenNodeBinaryExprOp::GtEq:
							op = ast::BinaryOp::GtEq;
							break;
						case ast::GreenNodeBinaryExprOp::Cmp:
							op = ast::BinaryOp::Cmp;
							break;
						default:
							std::terminate();
					}

					ast::AstNodePin<ast::BinaryExprNode> e = ast::make_ast_node<ast::BinaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					e->binary_op = op;

					ast::RedNodePtr lhs_node, rhs_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], lhs_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], rhs_node)));

					ast::AstNodePin<ast::AstNode> lhs, rhs;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, lhs_node, lhs)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, rhs_node, rhs)(sched));

					e->lhs = lhs.cast_to<ast::ExprNode>();
					e->rhs = rhs.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::Ternary: {
					ast::AstNodePin<ast::TernaryExprNode> e = ast::make_ast_node<ast::TernaryExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast::RedNodePtr cond_node, true_node, false_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], cond_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], true_node)));
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[2], false_node)));

					ast::AstNodePin<ast::AstNode> cond, tb, fb;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, cond_node, cond)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_node, tb)(sched));
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, false_node, fb)(sched));

					e->condition = cond.cast_to<ast::ExprNode>();
					e->true_branch = tb.cast_to<ast::ExprNode>();
					e->false_branch = fb.cast_to<ast::ExprNode>();

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::IdRef: {
					ast::AstNodePin<ast::IdRefExprNode> e = ast::make_ast_node<ast::IdRefExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast::RedNodePtr id_ref_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::IdRef)[0], id_ref_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_id_ref(state_allocator, sched, env, id_ref_node, e->id_ref)(sched));

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeExprKind::HeadedIdRef: {
					ast::AstNodePin<ast::HeadedIdRefExprNode> e = ast::make_ast_node<ast::HeadedIdRefExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], expr_node)));

						ast::AstNodePin<ast::AstNode> head_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, head_expr)(sched));
						e->head_expr = std::move(head_expr).cast_to<ast::ExprNode>();
					}

					ast::RedNodePtr id_ref_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::IdRef)[0], id_ref_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_id_ref(state_allocator, sched, env, id_ref_node, e->id_ref)(sched));

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::InitializerList: {
					ast::AstNodePin<ast::InitializerListExprNode> e = ast::make_ast_node<ast::InitializerListExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						ast::RedNodeChildIndices args_indices(state_allocator);

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								args_node->build_children(env->get_global()->get_allocator())));

						if (!args_indices.index_children(args_node))
							co_return gen_oom_error_option();

						for (auto i : args_indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									args_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (!e->elements.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return gen_oom_error_option();
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Call: {
					ast::AstNodePin<ast::CallExprNode> e = ast::make_ast_node<ast::CallExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr callee_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], callee_node)));

						ast::AstNodePin<ast::AstNode> callee_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, callee_node, callee_expr)(sched));
						e->target = std::move(callee_expr).cast_to<ast::ExprNode>();
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								args_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices args_indices(state_allocator);

						if (!args_indices.index_children(args_node))
							co_return gen_oom_error_option();

						for (auto i : args_indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									args_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (!e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return gen_oom_error_option();
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Subscript: {
					ast::AstNodePin<ast::SubscriptExprNode> e = ast::make_ast_node<ast::SubscriptExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr callee_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], callee_node)));

						ast::AstNodePin<ast::AstNode> callee_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, callee_node, callee_expr)(sched));
						e->target = std::move(callee_expr).cast_to<ast::ExprNode>();
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								args_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices args_indices(state_allocator);

						if (!args_indices.index_children(args_node))
							co_return gen_oom_error_option();

						for (auto i : args_indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									args_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (!e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return gen_oom_error_option();
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::New: {
					ast::AstNodePin<ast::NewExprNode> e = ast::make_ast_node<ast::NewExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr new_type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], new_type_node)));

						ast::AstNodePin<ast::TypeNameNode> tn;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, new_type_node, tn)(sched));
						e->target_type = tn;
					}

					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Args)[0], args_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								args_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices args_indices(state_allocator);

						if (!args_indices.index_children(args_node))
							co_return gen_oom_error_option();

						for (auto i : args_indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									args_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (!e->args.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return gen_oom_error_option();
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Cast: {
					ast::AstNodePin<ast::CastExprNode> e = ast::make_ast_node<ast::CastExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], type_node)));

						ast::AstNodePin<ast::TypeNameNode> tn;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, type_node, tn)(sched));
						e->target_type = tn;
					}

					{
						ast::RedNodePtr operand_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], operand_node)));

						ast::AstNodePin<ast::AstNode> operand;

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, operand_node, operand)(sched));

						e->operand = operand.cast_to<ast::ExprNode>();
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Match: {
					ast::AstNodePin<ast::MatchExprNode> e = ast::make_ast_node<ast::MatchExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					// Convert the condition node.
					{
						ast::RedNodePtr condition_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], condition_node)));

						ast::AstNodePin<ast::AstNode> condition;

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, condition_node, condition)(sched));

						e->condition = condition.cast_to<ast::ExprNode>();
					}

					// Check if there is explicit return type.
					if (auto index = indices.get_classified_indices(ast::GreenNodeKind::TypeName); index.size()) {
						ast::RedNodePtr type_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], type_node)));

						ast::AstNodePin<ast::TypeNameNode> tn;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, type_node, tn)(sched));
						e->return_type = tn;
					}

					// Lowering the branches.
					for (auto i : indices.get_classified_indices(ast::GreenNodeKind::MatchCase)) {
						ast::RedNodePtr case_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, i, case_node)));

						assert(case_node->is_green_node_facade());

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								case_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices case_indices(state_allocator);

						if (!case_indices.index_children(case_node))
							co_return gen_oom_error_option();

						ast::MatchExprBranch branch;

						if (auto index = case_indices.get_classified_indices(ast::TokenId::DefaultKeyword); index.size()) {
							ast::RedNodePtr default_keyword_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									case_node->get_child_node(env->get_global()->get_allocator(), index[0], default_keyword_node)));
							branch.sti_default_keyword = default_keyword_node->as_token()->index;
						} else {
							ast::RedNodePtr pattern_node, result_value_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									case_node->get_child_node(state_allocator, case_indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], pattern_node)));
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									case_node->get_child_node(state_allocator, case_indices.get_classified_indices(ast::GreenNodeKind::Expr)[1], result_value_node)));

							ast::AstNodePin<ast::AstNode> pattern, result_value;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, pattern_node, pattern)(sched));
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_value_node, result_value)(sched));

							branch.pattern = std::move(pattern).cast_to<ast::ExprNode>();
							branch.result_value = std::move(result_value).cast_to<ast::ExprNode>();
						}

						if (!e->branches.push_back(std::move(branch)))
							co_return gen_oom_error_option();
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Break: {
					ast::AstNodePin<ast::BreakExprNode> e = ast::make_ast_node<ast::BreakExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					// TODO: Add label support if implemented.

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Continue: {
					ast::AstNodePin<ast::ContinueExprNode> e = ast::make_ast_node<ast::ContinueExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					if (auto index = indices.get_classified_indices(ast::GreenNodeKind::Args); index.size())
					{
						ast::RedNodePtr args_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, index[0], args_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								args_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices args_indices(state_allocator);

						if (!args_indices.index_children(args_node))
							co_return gen_oom_error_option();

						for (auto i : args_indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									args_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::AstNodePin<ast::AstNode> entry;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, entry_node, entry)(sched));

							if (!e->continue_values.push_back(std::move(entry).cast_to<ast::ExprNode>()))
								co_return gen_oom_error_option();
						}
					}

					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Return: {
					ast::AstNodePin<ast::ReturnExprNode> e = ast::make_ast_node<ast::ReturnExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					if (auto index = indices.get_classified_indices(ast::GreenNodeKind::Expr); index.size())
					{
						ast::RedNodePtr result_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, index[0], result_node)));

						ast::AstNodePin<ast::AstNode> result_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_node, result_expr)(sched));
						e->return_value = std::move(result_expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Yield: {
					ast::AstNodePin<ast::YieldExprNode> e = ast::make_ast_node<ast::YieldExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr result_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], result_node)));

						ast::AstNodePin<ast::AstNode> result_expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, result_node, result_expr)(sched));
						e->return_value = std::move(result_expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				case slkc::ast::GreenNodeExprKind::Group: {
					ast::AstNodePin<ast::GroupExprNode> e = ast::make_ast_node<ast::GroupExprNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], expr_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
						e->operand = std::move(expr).cast_to<ast::ExprNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ast::GreenNodeKind::Stmt: {
			auto exdata = std::get<ast::StmtGreenNodeExData>(g->exdata);

			switch (exdata.stmt_kind) {
				case slkc::ast::GreenNodeStmtKind::IfStmt: {
					ast::AstNodePin<ast::IfStmtNode> e = ast::make_ast_node<ast::IfStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], expr_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
						e->condition = std::move(expr).cast_to<ast::ExprNode>();
					}

					auto stmt_index = indices.get_classified_indices(ast::GreenNodeKind::Stmt);
					{
						ast::RedNodePtr true_branch_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, stmt_index[0], true_branch_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_branch_node, expr)(sched));
						e->true_branch = std::move(expr).cast_to<ast::StmtNode>();
					}

					if (stmt_index.size() > 1) {
						ast::RedNodePtr false_branch_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, stmt_index[1], false_branch_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, false_branch_node, expr)(sched));
						e->false_branch = std::move(expr).cast_to<ast::StmtNode>();
					}
					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::ForStmt: {
					ast::AstNodePin<ast::ForStmtNode> e = ast::make_ast_node<ast::ForStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr var_bindings_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::VarBindings)[0], var_bindings_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								var_bindings_node->build_children(env->get_global()->get_allocator())));

						ast::RedNodeChildIndices var_bindings_indices(state_allocator);

						if (!var_bindings_indices.index_children(var_bindings_node))
							co_return gen_oom_error_option();

						for (auto i : var_bindings_indices.get_classified_indices(ast::GreenNodeKind::VarBinding)) {
							ast::RedNodePtr entry_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									var_bindings_node->get_child_node(state_allocator, i, entry_node)));

							assert(entry_node->is_green_node_facade());

							ast::BindingEntry binding;

							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_var_binding(state_allocator, sched, env, entry_node, binding)(sched));

							if (!e->loop_vars.push_back(std::move(binding)))
								co_return gen_oom_error_option();
						}
					}

					auto expr_index = indices.get_classified_indices(ast::GreenNodeKind::Expr);
					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, expr_index[0], expr_node)));

						{
							ast::AstNodePin<ast::AstNode> expr;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
							e->condition_expr = std::move(expr).cast_to<ast::ExprNode>();
						}

						if (expr_index.size() > 1) {
							for (size_t i = 1; i < expr_index.size(); ++i) {
								ast::RedNodePtr step_expr_node;
								SLKC_CO_RETURN_IF_COMP_ERROR(
									_green_node_op_result_to_comp_error(
										red_node->get_child_node(state_allocator, expr_index[i], step_expr_node)));

								assert(step_expr_node->is_green_node_facade());

								ast::AstNodePin<ast::AstNode> step_expr;
								SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, step_expr_node, step_expr)(sched));
								if (!e->step_exprs.push_back(std::move(step_expr).cast_to<ast::ExprNode>()))
									co_return gen_oom_error_option();
							}
						}
					}

					auto stmt_index = indices.get_classified_indices(ast::GreenNodeKind::Stmt);
					{
						ast::RedNodePtr true_branch_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, stmt_index[0], true_branch_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_branch_node, expr)(sched));
						e->body = std::move(expr).cast_to<ast::StmtNode>();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::WhileStmt: {
					ast::AstNodePin<ast::WhileStmtNode> e = ast::make_ast_node<ast::WhileStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					auto expr_index = indices.get_classified_indices(ast::GreenNodeKind::Expr);
					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, expr_index[0], expr_node)));

						{
							ast::AstNodePin<ast::AstNode> expr;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
							e->condition_expr = std::move(expr).cast_to<ast::ExprNode>();
						}
					}

					auto stmt_index = indices.get_classified_indices(ast::GreenNodeKind::Stmt);
					{
						ast::RedNodePtr true_branch_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, stmt_index[0], true_branch_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_branch_node, expr)(sched));
						e->body = std::move(expr).cast_to<ast::StmtNode>();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::DoWhileStmt: {
					ast::AstNodePin<ast::DoWhileStmtNode> e = ast::make_ast_node<ast::DoWhileStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					auto expr_index = indices.get_classified_indices(ast::GreenNodeKind::Expr);
					{
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, expr_index[0], expr_node)));

						{
							ast::AstNodePin<ast::AstNode> expr;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));
							e->condition_expr = std::move(expr).cast_to<ast::ExprNode>();
						}
					}

					auto stmt_index = indices.get_classified_indices(ast::GreenNodeKind::Stmt);
					{
						ast::RedNodePtr true_branch_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, stmt_index[0], true_branch_node)));

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, true_branch_node, expr)(sched));
						e->body = std::move(expr).cast_to<ast::StmtNode>();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::ExprStmt: {
					ast::AstNodePin<ast::ExprStmtNode> e = ast::make_ast_node<ast::ExprStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Expr)) {
						ast::RedNodePtr expr_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, i, expr_node)));

						assert(expr_node->is_green_node_facade());

						ast::AstNodePin<ast::AstNode> expr;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));

						if (!e->inner_exprs.push_back(std::move(expr).cast_to<ast::ExprNode>()))
							co_return gen_oom_error_option();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::LocalVarStmt: {
					ast::AstNodePin<ast::VarDefStmtNode> e = ast::make_ast_node<ast::VarDefStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					ast::RedNodePtr var_bindings_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::VarBindings)[0], var_bindings_node)));

					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							var_bindings_node->build_children(env->get_global()->get_allocator())));

					ast::RedNodeChildIndices var_bindings_indices(state_allocator);

					if (!var_bindings_indices.index_children(var_bindings_node))
						co_return gen_oom_error_option();

					for (auto i : var_bindings_indices.get_classified_indices(ast::GreenNodeKind::VarBinding)) {
						ast::RedNodePtr entry_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								var_bindings_node->get_child_node(state_allocator, i, entry_node)));

						assert(entry_node->is_green_node_facade());

						ast::BindingEntry binding;

						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_var_binding(state_allocator, sched, env, entry_node, binding)(sched));

						if (!e->bindings.push_back(std::move(binding)))
							co_return gen_oom_error_option();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::BlockStmt: {
					ast::AstNodePin<ast::BlockStmtNode> e = ast::make_ast_node<ast::BlockStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Stmt)) {
						ast::RedNodePtr stmt_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, i, stmt_node)));

						assert(stmt_node->is_green_node_facade());

						ast::AstNodePin<ast::AstNode> stmt;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, stmt_node, stmt)(sched));

						if (!e->inner_stmts.push_back(std::move(stmt).cast_to<ast::StmtNode>()))
							co_return gen_oom_error_option();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				case slkc::ast::GreenNodeStmtKind::SwitchStmt: {
					ast::AstNodePin<ast::SwitchStmtNode> e = ast::make_ast_node<ast::SwitchStmtNode>(env->get_global());

					if (!e)
						co_return _pin_fail_reason_to_comp_error(e.get_fail_reason());

					{
						ast::RedNodePtr condition_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], condition_node)));

						ast::AstNodePin<ast::AstNode> condition;
						SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, condition_node, condition)(sched));
						e->condition = std::move(condition).cast_to<ast::ExprNode>();
					}

					for (auto i : indices.get_classified_indices(ast::GreenNodeKind::SwitchCase)) {
						ast::RedNodePtr case_node;
						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								red_node->get_child_node(state_allocator, i, case_node)));

						SLKC_CO_RETURN_IF_COMP_ERROR(
							_green_node_op_result_to_comp_error(
								case_node->build_children(env->get_global()->get_allocator())));

						assert(case_node->is_green_node_facade());

						ast::RedNodeChildIndices case_indices(state_allocator);

						if (!case_indices.index_children(case_node))
							co_return gen_oom_error_option();

						ast::SwitchStmtBranch branch;

						if (auto index = case_indices.get_classified_indices(ast::TokenId::DefaultKeyword); index.size()) {
							ast::RedNodePtr default_keyword_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									case_node->get_child_node(env->get_global()->get_allocator(), index[0], default_keyword_node)));
							branch.sti_default_keyword = default_keyword_node->as_token()->index;
						} else {
							{
								ast::RedNodePtr pattern_node;
								SLKC_CO_RETURN_IF_COMP_ERROR(
									_green_node_op_result_to_comp_error(
										case_node->get_child_node(state_allocator, case_indices.get_classified_indices(ast::GreenNodeKind::Expr)[0], pattern_node)));

								ast::AstNodePin<ast::AstNode> pattern;
								SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, pattern_node, pattern)(sched));
								branch.pattern = std::move(pattern).cast_to<ast::ExprNode>();
							}
						}

						{
							ast::RedNodePtr body_node;
							SLKC_CO_RETURN_IF_COMP_ERROR(
								_green_node_op_result_to_comp_error(
									case_node->get_child_node(state_allocator, case_indices.get_classified_indices(ast::GreenNodeKind::Stmt)[0], body_node)));

							ast::AstNodePin<ast::AstNode> body;
							SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, body_node, body)(sched));
							branch.body = std::move(body).cast_to<ast::StmtNode>();
						}

						if (!e->branches.push_back(std::move(branch)))
							co_return gen_oom_error_option();
					}

					ast_node_out = e.cast_to<ast::AstNode>();

					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ast::GreenNodeKind::ClassDef: {
			ast::AstNodePin<ast::ClassNode> m = ast::make_ast_node<ast::ClassNode>(env->get_global());

			if (!m)
				co_return _pin_fail_reason_to_comp_error(m.get_fail_reason());

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				m->set_name(name_node->as_token()->source_text);
			}

			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::InheritanceSlot); index.size()) {
				ast::RedNodePtr inheritance_slot_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], inheritance_slot_node)));

				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						inheritance_slot_node->build_children(env->get_global()->get_allocator())));

				ast::RedNodeChildIndices slot_indices(state_allocator);

				if (!slot_indices.index_children(inheritance_slot_node))
					co_return gen_oom_error_option();

				ast::RedNodePtr tn_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						inheritance_slot_node->get_child_node(state_allocator, slot_indices.get_classified_indices(ast::GreenNodeKind::TypeName)[0], tn_node)));

				ast::AstNodePin<ast::TypeNameNode> tn;
				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, tn_node, tn)(sched));

				m->get_scope()->inherited_type = std::move(tn);
			}

			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::ImplItem); index.size()) {
				ast::RedNodePtr impl_list_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], impl_list_node)));

				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _lower_rg_impl_list_to_scope(state_allocator, sched, env, impl_list_node, m->get_scope())(sched));
			}

			if (auto index = indices.get_classified_indices(ast::TokenId::FinalKeyword); index.size()) {
				m->get_scope()->set_final(true);
			}

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m.cast_to<ast::MemberNode>())(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::FnDef:
		case ast::GreenNodeKind::FnDecl: {
			ast::AstNodePin<ast::FnNode> fn = ast::make_ast_node<ast::FnNode>(env->get_global());

			if (!fn)
				co_return _pin_fail_reason_to_comp_error(fn.get_fail_reason());

			ast::AstNodePin<ast::FnOverloadingNode> overload = ast::make_ast_node<ast::FnOverloadingNode>(env->get_global());

			if (!overload)
				co_return _pin_fail_reason_to_comp_error(overload.get_fail_reason());

			// Distinguish coroutine / operator functions from regular ones.
			if (indices.get_classified_indices(ast::TokenId::AsyncKeyword).size())
				overload->overloading_kind = ast::FnOverloadingKind::Coroutine;
			else if (indices.get_classified_indices(ast::TokenId::OperatorKeyword).size())
				overload->overloading_kind = ast::FnOverloadingKind::Operator;

			// Function name. Operator functions have no identifier; their name is
			// composed of the operator tokens, so the first token is used.
			if (auto index = indices.get_classified_indices(ast::TokenId::Id); index.size()) {
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], name_node)));

				fn->set_name(name_node->as_token()->source_text);
			} else {
				ast::RedNodePtr operator_name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::GreenNodeKind::OperatorName)[0], operator_name_node)));

				SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(operator_name_node->build_children(state_allocator)));

				for (const auto &child : operator_name_node->children) {
					if (child->is_token_facade()) {
						fn->set_name(child->as_token()->source_text);
						break;
					}
				}
			}

			// Parameters.
			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::VarBindings); index.size()) {
				ast::RedNodePtr var_bindings_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], var_bindings_node)));

				SLKC_CO_RETURN_IF_COMP_ERROR(_green_node_op_result_to_comp_error(var_bindings_node->build_children(state_allocator)));

				ast::RedNodeChildIndices var_bindings_indices(state_allocator);

				if (!var_bindings_indices.index_children(var_bindings_node))
					co_return gen_oom_error_option();

				for (auto i : var_bindings_indices.get_classified_indices(ast::GreenNodeKind::VarBinding)) {
					ast::RedNodePtr binding_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							var_bindings_node->get_child_node(state_allocator, i, binding_node)));

					assert(binding_node->is_green_node_facade());

					ast::BindingEntry binding;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_var_binding(state_allocator, sched, env, binding_node, binding)(sched));

					if (!overload->params.push_back(std::move(binding)))
						co_return gen_oom_error_option();
				}
			}

			// Overloading flags.
			if (indices.get_classified_indices(ast::TokenId::ConstKeyword).size())
				overload->overloading_flags |= ast::OVERLOADING_FLAG_CONST;
			if (indices.get_classified_indices(ast::TokenId::VirtualKeyword).size())
				overload->overloading_flags |= ast::OVERLOADING_FLAG_VIRTUAL;
			if (indices.get_classified_indices(ast::TokenId::OverrideKeyword).size())
				overload->overloading_flags |= ast::OVERLOADING_FLAG_OVERRIDE;

			// Return type. An overriding function may also declare the type being overridden before the return type,
			// e.g. override (BaseType) ReturnType.
			auto type_name_indices = indices.get_classified_indices(ast::GreenNodeKind::TypeName);

			if (type_name_indices.size()) {
				bool has_overridden_type = indices.get_classified_indices(ast::TokenId::OverrideKeyword).size() &&
										   indices.get_classified_indices(ast::TokenId::LParenthesis).size() > 1;

				size_t return_type_child_index = 0;

				if (has_overridden_type) {
					ast::RedNodePtr overridden_tn_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, type_name_indices[0], overridden_tn_node)));

					ast::AstNodePin<ast::TypeNameNode> overridden_type;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, overridden_tn_node, overridden_type)(sched));

					overload->overriden_type = std::move(overridden_type);

					return_type_child_index = 1;
				}

				if (type_name_indices.size() > return_type_child_index) {
					ast::RedNodePtr return_tn_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, type_name_indices[return_type_child_index], return_tn_node)));

					ast::AstNodePin<ast::TypeNameNode> tn;
					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_type_name(state_allocator, sched, env, return_tn_node, tn)(sched));
					overload->return_type = tn;
				}
			}

			// Function body. Declarations (semicolon-terminated) have no body.
			if (g->node_kind == ast::GreenNodeKind::FnDef) {
				ast::AstNodePin<ast::BlockStmtNode> body = ast::make_ast_node<ast::BlockStmtNode>(env->get_global());

				if (!body)
					co_return _pin_fail_reason_to_comp_error(body.get_fail_reason());

				for (auto i : indices.get_classified_indices(ast::GreenNodeKind::Stmt)) {
					ast::RedNodePtr stmt_node;
					SLKC_CO_RETURN_IF_COMP_ERROR(
						_green_node_op_result_to_comp_error(
							red_node->get_child_node(state_allocator, i, stmt_node)));

					assert(stmt_node->is_green_node_facade());

					ast::AstNodePin<ast::AstNode> stmt;

					SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, stmt_node, stmt)(sched));

					if (!body->inner_stmts.push_back(std::move(stmt).cast_to<ast::StmtNode>()))
						co_return gen_oom_error_option();
				}

				overload->body = std::move(body).cast_to<ast::BlockStmtNode>();
			}

			if (!fn->overloadings.push_back(overload.cast_to<ast::FnOverloadingNode>()))
				co_return gen_oom_error_option();

			ast_node_out = fn.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::InterfaceDef:
		case ast::GreenNodeKind::TraitDef:
		case ast::GreenNodeKind::StructDef: {
			ast::AstNodePin<ast::MemberNode> m;

			switch (g->node_kind) {
				case ast::GreenNodeKind::InterfaceDef: {
					ast::AstNodePin<ast::InterfaceNode> node = ast::make_ast_node<ast::InterfaceNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				case ast::GreenNodeKind::TraitDef: {
					ast::AstNodePin<ast::TraitNode> node = ast::make_ast_node<ast::TraitNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				case ast::GreenNodeKind::StructDef: {
					ast::AstNodePin<ast::StructNode> node = ast::make_ast_node<ast::StructNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				default:
					std::terminate();
			}

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				m->set_name(name_node->as_token()->source_text);
			}

			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::ImplItem); index.size()) {
				ast::RedNodePtr impl_list_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], impl_list_node)));

				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _lower_rg_impl_list_to_scope(state_allocator, sched, env, impl_list_node, m->get_scope())(sched));
			}

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m)(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::ExceptDef: {
			ast::AstNodePin<ast::ExceptNode> m = ast::make_ast_node<ast::ExceptNode>(env->get_global());

			if (!m)
				co_return _pin_fail_reason_to_comp_error(m.get_fail_reason());

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				m->set_name(name_node->as_token()->source_text);
			}

			// The inheritance slot of an exception declares its base exception type.
			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::InheritanceSlot); index.size()) {
				ast::RedNodePtr inheritance_slot_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], inheritance_slot_node)));

				ast::AstNodePtr<ast::TypeNameNode> inherited_type;

				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _lower_rg_inheritance_slot_to_type_name(state_allocator, sched, env, inheritance_slot_node, inherited_type)(sched));

				m->get_scope()->inherited_type = std::move(inherited_type);
			}

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m.cast_to<ast::MemberNode>())(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::ConstEnumDef:
		case ast::GreenNodeKind::ScopedEnumDef:
		case ast::GreenNodeKind::UnionEnumDef: {
			ast::AstNodePin<ast::MemberNode> m;

			switch (g->node_kind) {
				case ast::GreenNodeKind::ConstEnumDef: {
					ast::AstNodePin<ast::ConstEnumNode> node = ast::make_ast_node<ast::ConstEnumNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				case ast::GreenNodeKind::ScopedEnumDef: {
					ast::AstNodePin<ast::ScopedEnumNode> node = ast::make_ast_node<ast::ScopedEnumNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				case ast::GreenNodeKind::UnionEnumDef: {
					ast::AstNodePin<ast::UnionEnumNode> node = ast::make_ast_node<ast::UnionEnumNode>(env->get_global());

					if (!node)
						co_return _pin_fail_reason_to_comp_error(node.get_fail_reason());

					m = node.cast_to<ast::MemberNode>();
					break;
				}
				default:
					std::terminate();
			}

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				m->set_name(name_node->as_token()->source_text);
			}

			// The inheritance slot of an enumeration declares its underlying type.
			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::InheritanceSlot); index.size()) {
				ast::RedNodePtr inheritance_slot_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], inheritance_slot_node)));

				ast::AstNodePtr<ast::TypeNameNode> underlying_type;

				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _lower_rg_inheritance_slot_to_type_name(state_allocator, sched, env, inheritance_slot_node, underlying_type)(sched));

				m->get_scope()->underlying_type = std::move(underlying_type);
			}

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m)(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}
		case ast::GreenNodeKind::ConstAndScopedEnumItem: {
			ast::AstNodePin<ast::EnumItemNode> item = ast::make_ast_node<ast::EnumItemNode>(env->get_global());

			if (!item)
				co_return _pin_fail_reason_to_comp_error(item.get_fail_reason());

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				item->set_name(name_node->as_token()->source_text);
			}

			if (auto index = indices.get_classified_indices(ast::GreenNodeKind::Expr); index.size()) {
				ast::RedNodePtr expr_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], expr_node)));

				ast::AstNodePin<ast::AstNode> expr;
				SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_node_to_ast_node(state_allocator, sched, env, expr_node, expr)(sched));

				item->specified_value = std::move(expr).cast_to<ast::ExprNode>();
			}

			ast_node_out = item.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::UnionEnumCase: {
			ast::AstNodePin<ast::UnionEnumItemNode> item = ast::make_ast_node<ast::UnionEnumItemNode>(env->get_global());

			if (!item)
				co_return _pin_fail_reason_to_comp_error(item.get_fail_reason());

			{
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, indices.get_classified_indices(ast::TokenId::Id)[0], name_node)));

				item->set_name(name_node->as_token()->source_text);
			}

			ast_node_out = item.cast_to<ast::AstNode>();
			break;
		}

		case ast::GreenNodeKind::AttributeDef: {
			ast::AstNodePin<ast::AttributeNode> m = ast::make_ast_node<ast::AttributeNode>(env->get_global());

			if (!m)
				co_return _pin_fail_reason_to_comp_error(m.get_fail_reason());

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			if (auto index = indices.get_classified_indices(ast::TokenId::Id); index.size()) {
				ast::RedNodePtr name_node;
				SLKC_CO_RETURN_IF_COMP_ERROR(
					_green_node_op_result_to_comp_error(
						red_node->get_child_node(state_allocator, index[0], name_node)));

				m->set_name(name_node->as_token()->source_text);
			}

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m.cast_to<ast::MemberNode>())(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}
		case ast::GreenNodeKind::Module: {
			ast::AstNodePin<ast::ModuleNode> m = ast::make_ast_node<ast::ModuleNode>(env->get_global());

			if (!m)
				co_return _pin_fail_reason_to_comp_error(m.get_fail_reason());

			if (!m->alloc_scope())
				co_return gen_oom_error_option();

			// TODO: Lower the module name.

			SLKC_CO_RETURN_IF_COMP_ERROR(co_await _do_lower_rg_nodes_to_ast_members(state_allocator, sched, env, red_node, m.cast_to<ast::MemberNode>())(sched));

			ast_node_out = m.cast_to<ast::AstNode>();
			break;
		}
		default:
			std::terminate();
	}

	co_return peff::NULLOPT;
}

SLKC_API peff::Result<ast::AstNodePin<ast::AstNode>, CompilationError> comp::lower_rg_node_to_ast_node(peff::Alloc *state_allocator, comp::CompilationEnv *env, const PEFF_IN_REF ast::RedNodePtr &green_node) {
	ast::AstNodePin<ast::AstNode> node;

	CompilationCoroutineScheduler sched(state_allocator);

	SLKC_RETURN_IF_COMP_ERROR(_do_lower_rg_node_to_ast_node(state_allocator, &sched, env, green_node, node).resume(&sched));

	return node;
}
