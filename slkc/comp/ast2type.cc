#include "ast2type.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API CompilationCoroutine comp::lower_ast_type_name_to_type_ref(peff::Alloc *allocator, Global *global, const ast::AstNodePin<ast::TypeNameNode> &tn_in, TypeRef &tr_out) {
	switch (tn_in->get_tn_kind()) {
		case ast::TypeNameKind::Void:
			tr_out.set_kind(TypeKind::Void);
			break;
		case ast::TypeNameKind::I8:
			tr_out.set_kind(TypeKind::I8);
			break;
		case ast::TypeNameKind::I16:
			tr_out.set_kind(TypeKind::I16);
			break;
		case ast::TypeNameKind::I32:
			tr_out.set_kind(TypeKind::I32);
			break;
		case ast::TypeNameKind::I64:
			tr_out.set_kind(TypeKind::I64);
			break;
		case ast::TypeNameKind::ISize:
			tr_out.set_kind(TypeKind::ISize);
			break;
		case ast::TypeNameKind::U8:
			tr_out.set_kind(TypeKind::U8);
			break;
		case ast::TypeNameKind::U16:
			tr_out.set_kind(TypeKind::U16);
			break;
		case ast::TypeNameKind::U32:
			tr_out.set_kind(TypeKind::U32);
			break;
		case ast::TypeNameKind::U64:
			tr_out.set_kind(TypeKind::U64);
			break;
		case ast::TypeNameKind::USize:
			tr_out.set_kind(TypeKind::USize);
			break;
		case ast::TypeNameKind::F32:
			tr_out.set_kind(TypeKind::F32);
			break;
		case ast::TypeNameKind::F64:
			tr_out.set_kind(TypeKind::F64);
			break;
		case ast::TypeNameKind::String:
			tr_out.set_kind(TypeKind::String);
			break;
		case ast::TypeNameKind::Bool:
			tr_out.set_kind(TypeKind::Bool);
			break;
		case ast::TypeNameKind::Object:
			tr_out.set_kind(TypeKind::Object);
			break;
		case ast::TypeNameKind::Any:
			tr_out.set_kind(TypeKind::Any);
			break;
		case ast::TypeNameKind::Never:
			tr_out.set_kind(TypeKind::Never);
			break;
		case ast::TypeNameKind::Custom: {
			tr_out.set_kind(TypeKind::Custom);

			TypeDefPtr<CustomTypeDef> td = alloc_type_def<CustomTypeDef>(global);

			if (!td)
				co_return gen_oom_error_option();

			// TODO: Implement it after ID reference resolving is implemented.

			break;
		}
		case ast::TypeNameKind::Unpacking:
			tr_out.set_kind(TypeKind::Unpacking);
			// TODO: Implement it.
			break;
		default:
			// TODO: Implement it.
			std::terminate();
	}

	tr_out.set_const(tn_in->is_const());
	tr_out.set_final(tn_in->is_final());
	tr_out.set_nullability(tn_in->get_nullability());
	tr_out.set_shareability(tn_in->get_shareability());

	co_return peff::NULLOPT;
}
