#include "env.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API CompilationEnv::CompilationEnv(ast::Global *global, const ast::AstNodePin<ast::ModuleNode> &target_module) noexcept
	: _global(global),
	  _target_module(target_module),
	  _compilation_errors(global->get_allocator()) {
}
