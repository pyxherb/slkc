#ifndef _SLKC_COMP_IDREF_H_
#define _SLKC_COMP_IDREF_H_

#include "env.h"
#include <coroutine>

namespace slkc {
	namespace comp {
		SLKC_API CompilationCoroutine resolve_id_ref(
			peff::Alloc *state_allocator,
			CompilationEnv *env,
			PEFF_IN_REF const ast::IdRefView &view,
			const ast::AstNodePtr<ast::MemberNode> &initial_node,
			ast::AstNodePtr<ast::MemberNode> &node_out);
	}
}

#endif
