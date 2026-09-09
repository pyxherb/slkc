#include "env.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API CompilationEnv::CompilationEnv(ast::Global *global) noexcept
	: _global(global),
	  _compilation_errors(global->get_allocator()) {
}
