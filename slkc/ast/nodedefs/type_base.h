#ifndef _SLKC_AST_NODEDEFS_TYPE_BASE_H_
#define _SLKC_AST_NODEDEFS_TYPE_BASE_H_

#include "../utils.h"

namespace slkc {
	namespace ast {
		enum class TypeNameKind : uint8_t {
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

		class TypeNameDefNode : public Node {
		public:
			SLKC_API TypeNameDefNode(peff::Alloc *self_allocator, Global *global);
			SLKC_API TypeNameDefNode(const TypeNameDefNode &other, peff::Alloc *new_allocator, DuplicationContext &context);
			SLKC_API virtual ~TypeNameDefNode();
		};

		enum class TypeNameShareability : uint8_t {
			Unspecified = 0,
			Multi,
			Restrict,
			Synchronized,
		};

		struct TypeName final {
		private:
			NodePtr<TypeNameDefNode> _typename_def;

		public:
			TokenRange token_range;
			TokenIndex sti_final_token = INVALID_TOKEN_INDEX, sti_local_token = INVALID_TOKEN_INDEX, sti_nullable_token = INVALID_TOKEN_INDEX;

		private:
			TypeNameKind _tn_kind;
			bool _is_const : 1;
			
			bool _is_final : 1;
			
			bool _is_local : 1;
			
			TypeNameShareability shareability : 2 = TypeNameShareability::Unspecified;

			bool _is_nullable : 1;
			bool _is_ref : 1;
			bool _is_readonly_ref : 1;

		public:
			SLAKE_FORCEINLINE TypeName()
				: _tn_kind(TypeNameKind::Invalid),
				  _is_const(false),
				  _is_final(false),
				  _is_local(false),
				  _is_nullable(false),
				  _is_ref(false),
				  _is_readonly_ref(false) {}

			SLAKE_FORCEINLINE TypeName(TypeNameKind tn_kind, NodePtr<TypeNameDefNode> typename_def)
				: _tn_kind(tn_kind),
				  _typename_def(typename_def),
				  _is_const(false),
				  _is_final(false),
				  _is_local(false),
				  _is_nullable(false),
				  _is_ref(false),
				  _is_readonly_ref(false) {
			}

			TypeName(const TypeName &rhs) = default;
			TypeName(TypeName &&rhs) = default;

			TypeName &operator=(const TypeName &rhs) = default;
			TypeName &operator=(TypeName &&rhs) = default;

			SLAKE_FORCEINLINE NodePtr<TypeNameDefNode> get_def() const noexcept {
				return _typename_def;
			}

			template <typename T>
			SLAKE_FORCEINLINE NodePtr<T> get_typed_def() const noexcept {
				return _typename_def.cast_to<T>();
			}

			SLAKE_FORCEINLINE void set_def(const NodePtr<TypeNameDefNode> &typename_def) noexcept {
				_typename_def = typename_def;
			}

			/// @brief Get typename kind of the type name.
			///
			/// @return Type name kind of the type name.
			SLAKE_FORCEINLINE TypeNameKind get_typename_kind() const noexcept {
				return _tn_kind;
			}

			SLAKE_FORCEINLINE void set_typename_kind(TypeNameKind tn_kind) noexcept {
				_tn_kind = tn_kind;
			}

			/// @brief Check if the type name is with `const` modifier.
			///
			/// @return Whether the type name is with `const` modifier.
			SLAKE_FORCEINLINE bool is_const() const noexcept {
				return _is_const;
			}
				
			/// @brief Set if the type name is with `const` modifier.
			///
			/// @param b Whether the type name will be set to be with `const` modifier.
			SLAKE_FORCEINLINE void set_const(bool b) noexcept {
				_is_const = b;
			}

			/// @brief Check if the type name is with `final` modifier.
			///
			/// @return Whether the type name is with `final` modifier.
			SLAKE_FORCEINLINE bool is_final() const noexcept {
				return _is_final;
			}

			/// @brief Set if the type name is with `final` modifier.
			///
			/// @param b Whether the type name will be set to be with `final` modifier.
			SLAKE_FORCEINLINE void set_final(bool b) noexcept {
				_is_final = b;
			}
			
			
			SLAKE_FORCEINLINE TypeNameShareability get_shareability() const noexcept {
				return shareability;
			}

			SLAKE_FORCEINLINE void set_shareability(TypeNameShareability s) noexcept {
				shareability = s;
			}
			
			/// @brief Check if the type name is with `local` modifier.
			///
			/// @return Whether the type name is with `local` modifier.
			SLAKE_FORCEINLINE bool is_local() const noexcept {
				return _is_local;
			}

			/// @brief Set if the type name is with `local` modifier.
			///
			/// @param b Whether the type name will be set to be with `local` modifier.
			SLAKE_FORCEINLINE void set_local(bool b) noexcept {
				_is_local = b;
			}

			/// @brief Check if the type name is nullable.
			///
			/// @return Whether the type name is nullable.
			SLAKE_FORCEINLINE bool is_nullable() const noexcept {
				return _is_nullable;
			}

			/// @brief Set if the type name is nullable.
			///
			/// @param b Whether the type name will be set to be nullable.
			SLAKE_FORCEINLINE void set_nullable(bool b) noexcept {
				_is_nullable = b;
			}

			/// @brief Check if the type name is nullable.
			///
			/// @return Whether the type name is nullable.
			SLAKE_FORCEINLINE bool is_ref() const noexcept {
				return _is_ref;
			}

			/// @brief Set if the type name is nullable.
			///
			/// @param b Whether the type name will be set to be nullable.
			SLAKE_FORCEINLINE void set_ref(bool b) noexcept {
				_is_ref = b;
			}
			
			/// @brief Check if the type name is with `readonly` modifier.
			///
			/// @return Whether the type name is with `readonly` modifier.
			SLAKE_FORCEINLINE bool is_readonly_ref() const noexcept {
				return _is_readonly_ref;
			}

			/// @brief Set if the type name is with `readonly` modifier.
			///
			/// @param b Whether the type name will be set to be with `readonly` modifier.
			SLAKE_FORCEINLINE void set_readonly_ref(bool b) noexcept {
				_is_readonly_ref = b;
			}

			/// @brief Check if the type name is explicitly marked as `final` in the source.
			///
			/// @return Whether the type name is explicitly marked as `final` in the source.
			SLAKE_FORCEINLINE bool is_explicit_final() const noexcept {
				return sti_final_token != INVALID_TOKEN_INDEX;
			}

			/// @brief Check if the type name is explicitly marked as `local` in the source.
			///
			/// @return Whether the type name is explicitly marked as `local in the source.
			SLAKE_FORCEINLINE bool is_explicit_local() const noexcept {
				return sti_local_token != INVALID_TOKEN_INDEX;
			}

			/// @brief Check if the type name is explicitly marked as nullable in the source.
			///
			/// @return Whether the type name is explicitly marked as nullable in the source.
			SLAKE_FORCEINLINE bool is_explicit_nullable() const noexcept {
				return sti_nullable_token != INVALID_TOKEN_INDEX;
			}
		};

		SLKC_API DumpResult dump_typename(wandjson::ObjectValue *target_object, DumpContext &dump_context, const TypeName &tn, bool deep_dump);
	}
}

#endif
