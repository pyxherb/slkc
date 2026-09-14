#ifndef _SLKC_COMP_TYPE_TYPENAME_H_
#define _SLKC_COMP_TYPE_TYPENAME_H_

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

			Null,

			Bad
		};

		enum class TypeNameShareability : uint8_t {
			Unspecified = 0,
			Multi,
			Restrict,
			Synchronized,
		};

		class Type;

		struct CustomEntityTypeDefExData {
			ast::AstNodeIndex def_node_index;
		};

		struct UnpackingTypeDefExData {
			Type *unpackee_type;
		};

		struct FnTypeDefExData {
			Type *this_type;
			peff::DynArray<Type *> capturing_types;
			peff::DynArray<Type *> param_types;
			Type *return_type;
		};

		struct ArrayTypeDefExData {
			Type *element_type;
			uint32_t rank;
		};

		struct TupleTypeDefExData {
			peff::DynArray<Type *> element_types;
		};
		
		struct ParamTypesTypeDefExData {
			peff::DynArray<Type *> param_types;
		};

		struct EntityTypeDef {
		private:
			std::variant<std::monostate, CustomEntityTypeDefExData> _exdata;
			bool _is_const : 1;
			bool _is_final : 1;
			bool _is_local : 1;
			TypeNameShareability _shareability : 2;

		public:
			SLAKE_FORCEINLINE EntityTypeDef() : _is_const(false), _is_final(false), _is_local(false), _shareability(TypeNameShareability::Unspecified) {}

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

			SLAKE_FORCEINLINE bool is_local() const noexcept {
				return _is_local;
			}

			SLAKE_FORCEINLINE void set_local(bool flag) noexcept {
				_is_local = flag;
			}

			SLAKE_FORCEINLINE TypeNameShareability get_shareability() const noexcept {
				return _shareability;
			}

			SLAKE_FORCEINLINE void set_shareability(TypeNameShareability shareability) noexcept {
				_shareability = shareability;
			}
		};

		struct RefTypeDef {
		private:
			EntityTypeDef _entity_type_def;
			bool _ref_mutability : 1;
			bool _ref_locality : 1;

		public:
			SLAKE_FORCEINLINE const EntityTypeDef &get_entity_type() {
				return _entity_type_def;
			}

			SLAKE_FORCEINLINE void set_entity_type(const EntityTypeDef &def) {
				_entity_type_def = def;
			}

			SLAKE_FORCEINLINE bool is_readonly() const noexcept {
				return !_ref_mutability;
			}

			SLAKE_FORCEINLINE void set_readonly(bool flag) noexcept {
				_ref_mutability = !flag;
			}

			SLAKE_FORCEINLINE bool is_local() const noexcept {
				return _ref_locality;
			}

			SLAKE_FORCEINLINE void set_local(bool flag) noexcept {
				_ref_locality = flag;
			}
		};

		class Type final {
		private:
			std::variant<
				std::monostate,
				RefTypeDef,
				EntityTypeDef>
				_exdata;
			TypeKind _kind;

		public:
			SLAKE_FORCEINLINE Type(TypeKind kind) : _kind(kind) {}
			SLAKE_FORCEINLINE Type(RefTypeDef def) : _kind(TypeKind::Ref), _exdata(std::move(def)) {}
			SLAKE_FORCEINLINE Type(TypeKind kind, EntityTypeDef def) : _kind(kind), _exdata(std::move(def)) {}

			SLAKE_FORCEINLINE void set_kind(TypeKind kind) noexcept {
				_kind = kind;
			}

			SLAKE_FORCEINLINE TypeKind get_kind() const noexcept {
				return _kind;
			}

			SLAKE_FORCEINLINE const RefTypeDef& get_ref_type_def() const noexcept {
				return *std::get_if<RefTypeDef>(&_exdata);
			}

			SLAKE_FORCEINLINE RefTypeDef &get_ref_type_def() noexcept {
				return *std::get_if<RefTypeDef>(&_exdata);
			}
			
			SLAKE_FORCEINLINE const EntityTypeDef& get_entity_type_def() const noexcept {
				return *std::get_if<EntityTypeDef>(&_exdata);
			}

			SLAKE_FORCEINLINE EntityTypeDef &get_entity_type_def() noexcept {
				return *std::get_if<EntityTypeDef>(&_exdata);
			}
		};
	}
}

#endif
