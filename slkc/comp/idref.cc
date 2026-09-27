#include "idref.h"
#include "rg2ast.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API CompilationCoroutine comp::resolve_id_ref(
	peff::Alloc *state_allocator,
	CompilationEnv *env,
	PEFF_IN_REF const ast::IdRefView &view,
	const ast::AstNodePtr<ast::MemberNode> &initial_node,
	ast::AstNodePtr<ast::MemberNode> &node_out) {
	if (!(node_out = initial_node))
		co_return peff::NULLOPT;

	ast::AstNodePin<ast::MemberNode> pinned = node_out.pin();

	if (pinned.is_fail())
		co_return comp::_pin_fail_reason_to_comp_error(pinned.get_fail_reason());

	for (const auto &i : view) {
		{
			if (!(node_out = pinned->get_scope()->get_member(i.name)))
				co_return peff::NULLOPT;

			pinned = node_out.pin();

			if (pinned.is_fail())
				co_return comp::_pin_fail_reason_to_comp_error(pinned.get_fail_reason());
		}

		if (auto s = pinned->get_scope(); s) {
			if (s->generic_params.size()) {
				// TODO: Instantiate the generic member.
				std::terminate();
			}
		}

		if (!node_out)
			co_return peff::NULLOPT;
	}

	co_return peff::NULLOPT;
}
