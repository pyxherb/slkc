#ifndef _SLKC_COMP_TYPE_H_
#define _SLKC_COMP_TYPE_H_

#include <slkc/comp/env.h>

namespace slkc {
	namespace comp {
		enum class TypeKind : uint8_t {
			Invalid = 0,

			Void,
			I8,
			I16,
			I32,
			I64,
			ISize,
			U8,
			U16,
			U32,
			U64,
			USize,
			F32,
			F64,
			String,
			Bool,
			Object,
			Any,
			Null,
			Never,
			Custom,
			Unpacking,

			Fn,
			Array,
			Ref,
			Tuple,
			SIMD,
			ParamTypeList,
			UnpackedParams,
			UnpackedArgs,

			Bad
		};

		using TypeNullability = ast::TypeNameNullability;
		using TypeShareability = ast::TypeNameShareability;

		enum class TypeDefKind : uint8_t {
			Custom,
			Unpacking,
			Fn,
			Array,
			Tuple,
			ParamTypes,
			Ref,
		};

		class TypeDef {
		private:
			std::atomic_size_t _ref_count = 0;
			Global *_global;
			TypeDef *_next_destructible = nullptr;
			TypeDefIndex _type_def_index = INVALID_TYPE_DEF_INDEX;
			TypeDefKind _type_def_kind;

			friend class slkc::Global;

		public:
			SLAKE_FORCEINLINE TypeDef(Global *global, TypeDefKind type_def_kind) : _global(global), _type_def_kind(type_def_kind) {}
			~TypeDef() = default;

			virtual void dealloc() noexcept = 0;

			SLAKE_FORCEINLINE void set_type_def_index(TypeDefIndex index) noexcept {
				_type_def_index = index;
			}

			SLAKE_FORCEINLINE TypeDefIndex get_type_def_index() const noexcept {
				return _type_def_index;
			}

			SLAKE_FORCEINLINE TypeDefKind get_type_def_kind() const noexcept {
				return _type_def_kind;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE void inc_ref(size_t ignored = 0) noexcept {
				++_ref_count;
			}

			SLAKE_FORCEINLINE void dec_ref(size_t ignored = 0) noexcept {
				if (!--_ref_count) {
					// TODO: Add it onto the deferred deleting list.
				}
			}
		};

		template <typename T>
		using TypeDefPtr = peff::RcObjectPtr<T>;

		class TypeRef final {
		private:
			TypeDefPtr<TypeDef> _type_def;
			TypeKind _kind;
			bool _is_const : 1;
			bool _is_final : 1;
			TypeNullability _nullability : 2;
			TypeShareability _shareability : 2;

		public:
			SLAKE_FORCEINLINE TypeRef() : _kind(TypeKind::Invalid), _is_const(false), _is_final(false), _nullability(TypeNullability::Nullable), _shareability(TypeShareability::Unspecified) {}
			SLAKE_FORCEINLINE TypeRef(TypeKind kind) : _kind(kind), _is_const(false), _is_final(false), _nullability(TypeNullability::Nullable), _shareability(TypeShareability::Unspecified) {}
			SLAKE_FORCEINLINE TypeRef(TypeKind kind, const TypeDefPtr<TypeDef> &type_def) : _kind(TypeKind::Invalid), _type_def(type_def), _is_const(false), _is_final(false), _nullability(TypeNullability::Unspecified), _shareability(TypeShareability::Unspecified) {}

			SLAKE_FORCEINLINE void set_kind(TypeKind kind) noexcept {
				_kind = kind;
			}

			SLAKE_FORCEINLINE TypeKind get_kind() const noexcept {
				return _kind;
			}

			SLAKE_FORCEINLINE bool is_const() const noexcept {
				return _is_const;
			}

			SLAKE_FORCEINLINE void set_const(bool flag) noexcept {
				_is_const = flag;
			}

			SLAKE_FORCEINLINE bool is_final() const noexcept {
				return _is_final;
			}

			SLAKE_FORCEINLINE void set_final(bool flag) noexcept {
				_is_final = flag;
			}

			SLAKE_FORCEINLINE TypeNullability get_nullability() const noexcept {
				return _nullability;
			}

			SLAKE_FORCEINLINE void set_nullability(TypeNullability nullability) noexcept {
				_nullability = nullability;
			}

			SLAKE_FORCEINLINE TypeShareability get_shareability() const noexcept {
				return _shareability;
			}

			SLAKE_FORCEINLINE void set_shareability(TypeShareability shareability) noexcept {
				_shareability = shareability;
			}

			SLAKE_FORCEINLINE TypeDefPtr<TypeDef> get_type_def() const noexcept {
				return _type_def;
			}

			template <typename T>
			SLAKE_FORCEINLINE TypeDefPtr<T> get_typed_type_def() const noexcept {
				return TypeDefPtr<T>(static_cast<T *>(_type_def.get()));
			}

			template <typename T>
			SLAKE_FORCEINLINE void set_type_def(const TypeDefPtr<T> &td) noexcept {
				this->_type_def = td.cast_to<TypeDef>();
			}

			SLAKE_FORCEINLINE std::strong_ordering operator<=>(const TypeRef &rhs) const noexcept {
				if (auto result = _type_def <=> rhs._type_def; result != 0)
					return result;
				if (auto result = _kind <=> rhs._kind; result != 0)
					return result;
				if (auto result = _is_const <=> rhs._is_const; result != 0)
					return result;
				if (auto result = _is_final <=> rhs._is_final; result != 0)
					return result;
				if (auto result = _nullability <=> rhs._nullability; result != 0)
					return result;
				if (auto result = _shareability <=> rhs._shareability; result != 0)
					return result;
				return std::strong_ordering::equivalent;
			}
		};

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

			SLAKE_FORCEINLINE FnTypeDef(Global *global) : TypeDef(global, TypeDefKind::Fn), capture_types(global->get_allocator()), param_types(global->get_allocator()) {}

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
