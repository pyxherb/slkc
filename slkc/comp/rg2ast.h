#ifndef _SLKC_COMP_RG2AST_H_
#define _SLKC_COMP_RG2AST_H_

#include "env.h"
#include <coroutine>

namespace slkc {
	namespace comp {
		SLKC_API peff::Option<CompilationError> _pin_fail_reason_to_comp_error(ast::PinFailReason reason);
		SLKC_API peff::Option<CompilationError> _green_node_op_result_to_comp_error(ast::GreenNodeOperationResult result);

		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_var_binding(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::BindingEntry &binding_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::TypeName &type_name_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_id_ref(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::OwnedIdRef &id_ref_out);
		SLKC_API CompilationCoroutine _do_lower_rg_nodes_to_ast_members(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, const ast::AstNodePin<ast::MemberNode> &node_out);
		SLKC_API CompilationCoroutine _lower_rg_impl_list_to_scope(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &impl_list_node, ast::Scope *scope_out);
		SLKC_API CompilationCoroutine _lower_rg_inheritance_slot_to_type_name(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &inheritance_slot_node, ast::TypeName &type_name_out);
		SLKC_API CompilationCoroutine _lower_rg_var_binding_to_var_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, const ast::RedNodePtr &binding_node, bool is_var_binding, ast::AstNodePin<ast::VarNode> &var_node_out);
		SLKC_API CompilationCoroutine _do_lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationCoroutineScheduler *sched, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node, ast::AstNodePin<ast::AstNode> &ast_node_out);
		SLKC_API peff::Result<ast::AstNodePin<ast::AstNode>, CompilationError> lower_rg_node_to_ast_node(peff::Alloc *state_allocator, CompilationEnv *env, PEFF_IN_REF const ast::RedNodePtr &red_node);
	}
}

#endif
