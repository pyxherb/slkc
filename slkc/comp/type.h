#ifndef _SLKC_COMP_TYPE_H_
#define _SLKC_COMP_TYPE_H_

#include "type_base.h"

namespace slkc {
	namespace comp {
		class CustomTypeDef final : public TypeDef {
		public:
			ast::AstNodePtr<ast::MemberNode> type_src;

			SLAKE_FORCEINLINE CustomTypeDef(Global *global) : TypeDef(global, TypeDefKind::Custom) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class UnpackingTypeDef final : public TypeDef {
		public:
			TypeRef unpackee_type;

			SLAKE_FORCEINLINE UnpackingTypeDef(Global *global) : TypeDef(global, TypeDefKind::Unpacking) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class FnTypeDef final : public TypeDef {
		public:
			TypeRef this_type;
			peff::DynArray<TypeRef> capture_types;
			peff::DynArray<TypeRef> param_types;
			TypeRef return_type;

			SLAKE_API FnTypeDef(Global *global);

			SLKC_API virtual void dealloc() noexcept override;
		};

		class ArrayTypeDef final : public TypeDef {
		public:
			TypeRef element_type;
			uint32_t rank;

			SLAKE_FORCEINLINE ArrayTypeDef(Global *global) : TypeDef(global, TypeDefKind::Fn) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class TupleTypeDef final : public TypeDef {
		public:
			peff::DynArray<TypeRef> element_types;

			SLAKE_FORCEINLINE TupleTypeDef(Global *global) : TypeDef(global, TypeDefKind::Tuple), element_types(global->get_allocator()) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class ParamTypesTypeDef final : public TypeDef {
		public:
			peff::DynArray<TypeRef> param_types;

			SLAKE_FORCEINLINE ParamTypesTypeDef(Global *global) : TypeDef(global, TypeDefKind::ParamTypes), param_types(global->get_allocator()) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		struct RefTypeDef final : public TypeDef {
		public:
			TypeRef entity_type;
			bool is_readonly : 1;

			SLAKE_FORCEINLINE RefTypeDef(Global *global) : TypeDef(global, TypeDefKind::Ref), is_readonly(false) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		template<typename T>
		SLAKE_FORCEINLINE T* alloc_type_def(Global *global) {
			return peff::alloc_and_construct<T>(global->get_allocator(), alignof(T), global);
		}
	}
}

#endif
