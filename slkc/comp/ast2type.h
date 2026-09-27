#ifndef _SLKC_COMP_AST2TYPE_H_
#define _SLKC_COMP_AST2TYPE_H_

#include "type.h"
#include "env.h"
#include <slkc/ast/nodedefs/class.h>

namespace slkc {
	namespace comp {
		SLKC_API CompilationCoroutine lower_ast_type_name_to_type_ref(peff::Alloc *allocator, Global *global, const ast::AstNodePin<ast::TypeNameNode> &tn_in, TypeRef &tr_out);
	}
}

#endif
