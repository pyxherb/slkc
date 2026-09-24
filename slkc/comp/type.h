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

		enum class TypeNameShareability : uint8_t {
			Unspecified = 0,
			Multi,
			Restrict,
			Synchronized,
		};

		class TypeDef {
		private:
			Global *_global;
			peff::RcObjectPtr<peff::Alloc> _allocator;
			TypeDef *_next_destructible = nullptr;
			TypeDefIndex _type_def_index = INVALID_TYPE_DEF_INDEX;

			friend class slkc::Global;

		public:
			SLAKE_FORCEINLINE TypeDef(Global *global) : _allocator(global->get_allocator()), _global(global) {}
			~TypeDef() = default;

			virtual void dealloc() noexcept = 0;

			SLAKE_FORCEINLINE void set_type_def_index(TypeDefIndex index) noexcept {
				_type_def_index = index;
			}

			SLAKE_FORCEINLINE TypeDefIndex get_type_def_index() const noexcept {
				return _type_def_index;
			}
		};

		template <typename T>
		class TypeDefPin final {
		private:
			using ThisType = TypeDefPin<T>;
			TypeDefIndex _type_def_index;
			Global *_global;
			union {
				T *_ptr;
				PinFailReason _fail_reason;
			};

			template <typename T>
			friend class TypeDefPtr;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, TypeDefIndex type_def_index) {
				_global = global;
				_type_def_index = type_def_index;
				global->pin_type_def(type_def_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_global && _type_def_index != INVALID_TYPE_DEF_INDEX)
					_global->unpin_type_def(_type_def_index);
			}

			SLAKE_FORCEINLINE TypeDefPin() : _global(nullptr), _type_def_index(INVALID_TYPE_DEF_INDEX), _ptr(nullptr) {}
			SLAKE_FORCEINLINE explicit TypeDefPin(Global *global, TypeDefIndex type_def_index, T *ptr) : _global(global), _type_def_index(type_def_index), _ptr(ptr) {
			}
			SLAKE_FORCEINLINE explicit TypeDefPin(PinFailReason reason) : _global(nullptr), _type_def_index(INVALID_TYPE_DEF_INDEX), _fail_reason(reason) {
			}
			SLAKE_FORCEINLINE ~TypeDefPin() {
				reset();
			}

			SLAKE_FORCEINLINE TypeDefPin(const ThisType &rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index), _ptr(rhs._ptr) {
				_global->pin_type_def(_type_def_index);
			}
			SLAKE_FORCEINLINE TypeDefPin(ThisType &&rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index), _ptr(rhs._ptr) {
				rhs._global = nullptr;
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;
				rhs._ptr = nullptr;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._type_def_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_ptr = rhs._ptr;
				_type_def_index = rhs._type_def_index;
				rhs._global = nullptr;
				rhs._ptr = nullptr;
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE T *get() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE TypeDefIndex get_index() const noexcept {
				return _type_def_index;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE T *operator->() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_type_def_index > rhs._type_def_index)
					return 1;
				if (_type_def_index < rhs._type_def_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index < rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index > rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index == rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index != rhs._type_def_index;
			}

			SLAKE_FORCEINLINE explicit operator bool() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE bool is_fail() const noexcept {
				return !_global;
			}

			SLAKE_FORCEINLINE PinFailReason get_fail_reason() const noexcept {
				return _fail_reason;
			}

			template <typename T1>
			SLAKE_FORCEINLINE TypeDefPin<T1> cast_to() const noexcept {
				_global->pin_type_def(_type_def_index);
				return TypeDefPin<T1>(_global, _type_def_index, static_cast<T1 *>(_ptr));
			}
		};

		template <typename T>
		class TypeDefPtr final {
		private:
			using ThisType = TypeDefPtr<T>;
			TypeDefIndex _type_def_index;
			Global *_global;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, TypeDefIndex type_def_index) {
				_global = global;
				_type_def_index = type_def_index;
				global->ref_type_def(type_def_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_type_def_index != INVALID_TYPE_DEF_INDEX)
					_global->unref_type_def(_type_def_index);
			}

			SLAKE_FORCEINLINE TypeDefPtr() : _global(nullptr), _type_def_index(INVALID_TYPE_DEF_INDEX) {}
			SLAKE_FORCEINLINE TypeDefPtr(const TypeDefPin<T> &pin) : _global(pin.get_global()), _type_def_index(pin.get_index()) {
				if (_global)
					assert(_type_def_index != INVALID_TYPE_DEF_INDEX);
				_global->ref_type_def(_type_def_index);
			}
			SLAKE_FORCEINLINE explicit TypeDefPtr(Global *global, TypeDefIndex type_def_index) : _global(global), _type_def_index(type_def_index) {
				if (_global)
					assert(_type_def_index != INVALID_TYPE_DEF_INDEX);
				global->ref_type_def(type_def_index);
			}
			SLAKE_FORCEINLINE ~TypeDefPtr() {
				reset();
			}

			SLAKE_FORCEINLINE TypeDefPtr(const ThisType &rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index) {
				if (_global)
					_global->ref_type_def(_type_def_index);
			}
			SLAKE_FORCEINLINE TypeDefPtr(ThisType &&rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index) {
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._type_def_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_type_def_index = rhs._type_def_index;
				rhs._global = nullptr;
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE TypeDefIndex get_index() const noexcept {
				return _type_def_index;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			SLAKE_FORCEINLINE TypeDefPin<T> pin() const noexcept {
				auto result = _global->pin_type_def(_type_def_index);
				if (result.is_error())
					return TypeDefPin<T>(std::move(result).error());
				return TypeDefPin<T>(_global, _type_def_index, static_cast<T *>(std::move(result).value()));
			}

			SLAKE_FORCEINLINE static TypeDefPtr<T> from_pin(TypeDefPin<T> pin) noexcept {
				return TypeDefPtr<T>(pin._global, pin._type_def_index);
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_type_def_index > rhs._type_def_index)
					return 1;
				if (_type_def_index < rhs._type_def_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index < rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index > rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index == rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index != rhs._type_def_index;
			}

			SLAKE_FORCEINLINE explicit operator bool() const noexcept {
				return _type_def_index != INVALID_TYPE_DEF_INDEX;
			}

			template <typename T1>
			SLAKE_FORCEINLINE TypeDefPtr<T1> cast_to() const noexcept {
				static_assert(std::is_convertible_v<T *, T1 *>);

				return TypeDefPtr<T1>(_global, _type_def_index);
			}
		};

		template <typename T>
		class TypeDefWeakPtr final {
		private:
			using ThisType = TypeDefWeakPtr<T>;
			TypeDefIndex _type_def_index;
			Global *_global;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, TypeDefIndex type_def_index) {
				_global = global;
				_type_def_index = type_def_index;
				global->ref_type_def_weak(type_def_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_type_def_index != INVALID_TYPE_DEF_INDEX)
					_global->unref_type_def_weak(_type_def_index);
			}

			SLAKE_FORCEINLINE TypeDefWeakPtr() : _global(nullptr), _type_def_index(INVALID_TYPE_DEF_INDEX) {}
			SLAKE_FORCEINLINE TypeDefWeakPtr(const TypeDefPtr<T> &ptr) : _global(ptr.get_global()), _type_def_index(ptr.get_index()) {
				if (_global)
					assert(_type_def_index != INVALID_TYPE_DEF_INDEX);
				_global->ref_type_def_weak(_type_def_index);
			}
			SLAKE_FORCEINLINE explicit TypeDefWeakPtr(Global *global, TypeDefIndex type_def_index) : _global(global), _type_def_index(type_def_index) {
				if (_global)
					assert(_type_def_index != INVALID_TYPE_DEF_INDEX);
				global->ref_type_def_weak(type_def_index);
			}
			SLAKE_FORCEINLINE ~TypeDefWeakPtr() {
				reset();
			}

			SLAKE_FORCEINLINE TypeDefWeakPtr(const ThisType &rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index) {
				if (_global)
					_global->ref_type_def_weak(_type_def_index);
			}
			SLAKE_FORCEINLINE TypeDefWeakPtr(ThisType &&rhs) noexcept : _global(rhs._global), _type_def_index(rhs._type_def_index) {
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._type_def_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_type_def_index = rhs._type_def_index;
				rhs._global = nullptr;
				rhs._type_def_index = INVALID_TYPE_DEF_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE TypeDefIndex get_index() const noexcept {
				return _type_def_index;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			SLAKE_FORCEINLINE TypeDefPtr<T> reclaim() const noexcept {
				if (_global->try_ref_type_def(_type_def_index))
					return TypeDefPtr<T>(_global, _type_def_index);
				return {};
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_type_def_index > rhs._type_def_index)
					return 1;
				if (_type_def_index < rhs._type_def_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index < rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index > rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index == rhs._type_def_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _type_def_index != rhs._type_def_index;
			}

			SLAKE_FORCEINLINE explicit operator bool() const noexcept {
				return reclaim();
			}

			template <typename T1>
			SLAKE_FORCEINLINE TypeDefWeakPtr<T1> cast_to() const noexcept {
				static_assert(std::is_convertible_v<T *, T1 *>);

				return TypeDefWeakPtr<T1>(_global, _type_def_index);
			}
		};

		class TypeRef final {
		private:
			TypeDefPtr<TypeDef> _type_def;
			TypeKind _kind;
			bool _is_const : 1;
			bool _is_final : 1;
			bool _is_nullable : 1;
			TypeNameShareability _shareability : 2;

		public:
			SLAKE_FORCEINLINE TypeRef() : _kind(TypeKind::Invalid), _is_const(false), _is_final(false), _is_nullable(false), _shareability(TypeNameShareability::Unspecified) {}
			SLAKE_FORCEINLINE TypeRef(TypeKind kind) : _kind(kind), _is_const(false), _is_final(false), _is_nullable(false), _shareability(TypeNameShareability::Unspecified) {}
			SLAKE_FORCEINLINE TypeRef(TypeKind kind, const TypeDefPtr<TypeDef> &type_def) : _kind(TypeKind::Invalid), _type_def(type_def), _is_const(false), _is_final(false), _is_nullable(false), _shareability(TypeNameShareability::Unspecified) {}

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

			SLAKE_FORCEINLINE TypeNameShareability get_shareability() const noexcept {
				return _shareability;
			}

			SLAKE_FORCEINLINE void set_shareability(TypeNameShareability shareability) noexcept {
				_shareability = shareability;
			}

			SLAKE_FORCEINLINE TypeDefPtr<TypeDef> get_type_def() const noexcept {
				return _type_def;
			}

			template<typename T>
			SLAKE_FORCEINLINE TypeDefPtr<T> get_typed_type_def() const noexcept {
				return _type_def.cast_to<T>;
			}

			template<typename T>
			SLAKE_FORCEINLINE void set_type_def(const TypeDefPtr<T> &td) noexcept {
				this->_type_def = td.cast_to<TypeDef>();
			}
		};

		class CustomTypeDef final : public TypeDef {
		public:
			ast::AstNodePtr<ast::MemberNode> type_src;

			SLAKE_FORCEINLINE CustomTypeDef(Global *global) : TypeDef(global) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class UnpackingTypeDef final : public TypeDef {
		public:
			TypeRef unpackee_type;

			SLAKE_FORCEINLINE UnpackingTypeDef(Global *global) : TypeDef(global) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class FnTypeDef final : public TypeDef {
		public:
			TypeRef this_type;
			peff::DynArray<TypeRef> capture_types;
			peff::DynArray<TypeRef> param_types;
			TypeRef return_type;

			SLAKE_FORCEINLINE FnTypeDef(Global *global) : TypeDef(global), capture_types(global->get_allocator()), param_types(global->get_allocator()) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class ArrayTypeDef final : public TypeDef {
		public:
			TypeRef element_type;
			uint32_t rank;

			SLAKE_FORCEINLINE ArrayTypeDef(Global *global) : TypeDef(global) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class TupleTypeDef final : public TypeDef {
		public:
			peff::DynArray<TypeRef> element_types;

			SLAKE_FORCEINLINE TupleTypeDef(Global *global) : TypeDef(global), element_types(global->get_allocator()) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		class ParamTypesTypeDef final : public TypeDef {
		public:
			peff::DynArray<TypeRef> param_types;

			SLAKE_FORCEINLINE ParamTypesTypeDef(Global *global) : TypeDef(global), param_types(global->get_allocator()) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		struct RefTypeDef final : public TypeDef {
		public:
			TypeRef _entity_type_def;
			bool _is_readonly : 1;

			SLAKE_FORCEINLINE RefTypeDef(Global *global) : TypeDef(global), _is_readonly(false) {}

			SLKC_API virtual void dealloc() noexcept override;
		};

		template<typename T>
		SLAKE_FORCEINLINE TypeDefPin<T> make_type_def(Global *global) {
			TypeDef *node = peff::alloc_and_construct<TypeDef>(global->get_allocator(), alignof(TypeDef), global);
			if (!node)
				return TypeDefPin<T>();
			peff::ScopeGuard sg([global, node]() noexcept {
				peff::destroy_and_release<TypeDef>(global->get_allocator(), node, alignof(TypeDef));
			});

			{
				auto result = global->map_type_def(node);
				if (!result.has_value())
					return TypeDefPin<T>(PinFailReason::OutOfMemory);
				if (result.value() == INVALID_TYPE_DEF_INDEX)
					return TypeDefPin<T>(PinFailReason::OutOfNodeIndex);
			}

			{
				auto result = global->pin_type_def(node->get_type_def_index());
				assert(!result.is_error());
			}

			sg.release();

			return TypeDefPin<T>(global, node->get_type_def_index(), node);
		}
	}
}

#endif
