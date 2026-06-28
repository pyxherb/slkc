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

		struct TypeName final {
		private:
			NodePtr<TypeNameDefNode> _typename_def;

		public:
			TokenRange token_range;
			TokenIndex sti_final_token = INVALID_TOKEN_INDEX, sti_local_token = INVALID_TOKEN_INDEX, sti_nullable_token = INVALID_TOKEN_INDEX;

		private:
			TypeNameKind _tn_kind;
			bool _is_final = false;
			bool _is_local = false;
			bool _is_nullable = false;

		public:
			PEFF_FORCEINLINE TypeName() : _tn_kind(TypeNameKind::Invalid) {}

			PEFF_FORCEINLINE TypeName(TypeNameKind tn_kind, NodePtr<TypeNameDefNode> typename_def) : _tn_kind(tn_kind), _typename_def(typename_def) {
			}

			TypeName(const TypeName &rhs) = default;
			TypeName(TypeName &&rhs) = default;

			TypeName &operator=(const TypeName &rhs) = default;
			TypeName &operator=(TypeName &&rhs) = default;

			PEFF_FORCEINLINE NodePtr<TypeNameDefNode> get_def() const noexcept {
				return _typename_def;
			}

			template <typename T>
			PEFF_FORCEINLINE NodePtr<T> get_typed_def() const noexcept {
				return _typename_def.cast_to<T>();
			}

			PEFF_FORCEINLINE void set_def(const NodePtr<TypeNameDefNode> &typename_def) noexcept {
				_typename_def = typename_def;
			}

			/// @brief Get typename kind of the type name.
			///
			/// @return Type name kind of the type name.
			PEFF_FORCEINLINE TypeNameKind get_typename_kind() const noexcept {
				return _tn_kind;
			}

			PEFF_FORCEINLINE void set_typename_kind(TypeNameKind tn_kind) noexcept {
				_tn_kind = tn_kind;
			}

			/// @brief Check if the type name is with `final` modifier.
			///
			/// @return Whether the type name is with `final` modifier.
			PEFF_FORCEINLINE bool is_final() const noexcept {
				return _is_final;
			}

			/// @brief Set if the type name is with `final` modifier.
			///
			/// @param b Whether the type name will be set to be with `final` modifier.
			PEFF_FORCEINLINE void set_final(bool b) noexcept {
				_is_final = b;
			}

			/// @brief Check if the type name is with `local` modifier.
			///
			/// @return Whether the type name is with `local` modifier.
			PEFF_FORCEINLINE bool is_local() const noexcept {
				return _is_local;
			}

			/// @brief Set if the type name is with `local` modifier.
			///
			/// @param b Whether the type name is with `local` modifier.
			PEFF_FORCEINLINE void set_local(bool b) noexcept {
				_is_local = b;
			}

			/// @brief Check if the type name is nullable.
			///
			/// @return Whether the type name is nullable.
			PEFF_FORCEINLINE bool is_nullable() const noexcept {
				return _is_nullable;
			}

			/// @brief Set if the type name is nullable.
			///
			/// @param b Whether the type name will be set to be nullable.
			PEFF_FORCEINLINE void set_nullable(bool b) noexcept {
				_is_nullable = b;
			}

			/// @brief Check if the type name is explicitly marked as `final` in the source.
			///
			/// @return Whether the type name is explicitly marked as `final` in the source.
			PEFF_FORCEINLINE bool is_explicit_final() const noexcept {
				return sti_final_token != INVALID_TOKEN_INDEX;
			}

			/// @brief Check if the type name is explicitly marked as `local` in the source.
			///
			/// @return Whether the type name is explicitly marked as `local in the source.
			PEFF_FORCEINLINE bool is_explicit_local() const noexcept {
				return sti_local_token != INVALID_TOKEN_INDEX;
			}

			/// @brief Check if the type name is explicitly marked as nullable in the source.
			///
			/// @return Whether the type name is explicitly marked as nullable in the source.
			PEFF_FORCEINLINE bool is_explicit_nullable() const noexcept {
				return sti_nullable_token != INVALID_TOKEN_INDEX;
			}
		};

		SLKC_API DumpResult dump_typename(wandjson::ObjectValue *target_object, DumpContext &dump_context, const TypeName &tn, bool deep_dump);
	}
}

#endif
