#include "type.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API void CustomTypeDef::dealloc() noexcept {
	peff::destroy_and_release<CustomTypeDef>(get_global()->get_allocator(), this, alignof(CustomTypeDef));
}

SLKC_API void UnpackingTypeDef::dealloc() noexcept {
	peff::destroy_and_release<UnpackingTypeDef>(get_global()->get_allocator(), this, alignof(UnpackingTypeDef));
}

SLKC_API void FnTypeDef::dealloc() noexcept {
	peff::destroy_and_release<FnTypeDef>(get_global()->get_allocator(), this, alignof(FnTypeDef));
}

SLKC_API void ArrayTypeDef::dealloc() noexcept {
	peff::destroy_and_release<ArrayTypeDef>(get_global()->get_allocator(), this, alignof(ArrayTypeDef));
}

SLKC_API void TupleTypeDef::dealloc() noexcept {
	peff::destroy_and_release<TupleTypeDef>(get_global()->get_allocator(), this, alignof(TupleTypeDef));
}

SLKC_API void ParamTypesTypeDef::dealloc() noexcept {
	peff::destroy_and_release<ParamTypesTypeDef>(get_global()->get_allocator(), this, alignof(ParamTypesTypeDef));
}

SLKC_API void RefTypeDef::dealloc() noexcept {
	peff::destroy_and_release<RefTypeDef>(get_global()->get_allocator(), this, alignof(RefTypeDef));
}

SLKC_API std::strong_ordering comp::_compare_type_defs(const TypeDef *lhs, const TypeDef *rhs) {
	assert(lhs->get_global() == rhs->get_global());

	if (auto result = lhs->get_type_def_index() <=> rhs->get_type_def_index(); result != 0)
		return result;

	if (auto result = lhs->get_type_def_kind() <=> rhs->get_type_def_kind(); result != 0)
		return result;

	switch (lhs->get_type_def_kind()) {
		case TypeDefKind::Custom: {
			const CustomTypeDef *l = static_cast<const CustomTypeDef *>(lhs), *r = static_cast<const CustomTypeDef *>(rhs);

			if (auto result = l->type_src <=> r->type_src; result != 0)
				return result;
			break;
		}
		case TypeDefKind::Unpacking: {
			const CustomTypeDef *l = static_cast<const CustomTypeDef *>(lhs), *r = static_cast<const CustomTypeDef *>(rhs);

			if (auto result = l->type_src <=> r->type_src; result != 0)
				return result;
			break;
		}
		case TypeDefKind::Fn: {
			const FnTypeDef *l = static_cast<const FnTypeDef *>(lhs), *r = static_cast<const FnTypeDef *>(rhs);

			if (auto result = l->this_type <=> r->this_type; result != 0)
				return result;

			if (auto result = l->capture_types.size() <=> r->capture_types.size(); result != 0)
				return result;

			for (size_t i = 0; i < l->capture_types.size(); ++i) {
				if (auto result = l->capture_types[i] <=> r->capture_types[i]; result != 0)
					return result;
			}

			if (auto result = l->param_types.size() <=> r->param_types.size(); result != 0)
				return result;

			for (size_t i = 0; i < l->param_types.size(); ++i) {
				if (auto result = l->param_types[i] <=> r->param_types[i]; result != 0)
					return result;
			}

			if (auto result = l->return_type <=> r->return_type; result != 0)
				return result;
			break;
		}
		case TypeDefKind::Array: {
			const ArrayTypeDef *l = static_cast<const ArrayTypeDef *>(lhs), *r = static_cast<const ArrayTypeDef *>(rhs);

			if (auto result = l->element_type <=> r->element_type; result != 0)
				return result;

			if (auto result = l->rank <=> r->rank; result != 0)
				return result;
			break;
		}
		case TypeDefKind::Tuple: {
			const TupleTypeDef *l = static_cast<const TupleTypeDef *>(lhs), *r = static_cast<const TupleTypeDef *>(rhs);

			for (size_t i = 0; i < l->element_types.size(); ++i) {
				if (auto result = l->element_types[i] <=> r->element_types[i]; result != 0)
					return result;
			}
			break;
		}
		case TypeDefKind::ParamTypes: {
			const ParamTypesTypeDef *l = static_cast<const ParamTypesTypeDef *>(lhs), *r = static_cast<const ParamTypesTypeDef *>(rhs);

			for (size_t i = 0; i < l->param_types.size(); ++i) {
				if (auto result = l->param_types[i] <=> r->param_types[i]; result != 0)
					return result;
			}
			break;
		}
		case TypeDefKind::Ref: {
			const RefTypeDef *l = static_cast<const RefTypeDef *>(lhs), *r = static_cast<const RefTypeDef *>(rhs);

			if (auto result = l->entity_type <=> r->entity_type; result != 0)
				return result;

			if (auto result = l->is_readonly <=> r->is_readonly; result != 0)
				return result;
			break;
		}
		default:
			std::terminate();
	}

	return std::strong_ordering::equivalent;
}
