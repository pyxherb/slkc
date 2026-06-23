#ifndef _SLKC_AST_TYPENAME_BASE_H_
#define _SLKC_AST_TYPENAME_BASE_H_

#include "document.h"

namespace slkc {
	enum class TypeNameKind : uint8_t {
		Void = 0,
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
		TempRef,
		Tuple,
		SIMD,
		ParamTypeList,
		UnpackedParams,
		UnpackedArgs,

		Null,

		BCCustom,

		Bad
	};

	class TypeNameNode : public AstNode {
	private:
		const TypeNameKind tn_kind;

		bool _is_final = false;
		bool _is_local = false;
		bool _is_nullable = false;

	public:
		size_t idx_final_token = SIZE_MAX, idx_local_token = SIZE_MAX, idx_nullable_token = SIZE_MAX;

		SLKC_API TypeNameNode(TypeNameKind tn_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API TypeNameNode(const TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context);
		SLKC_API virtual ~TypeNameNode();

		/// @brief Get typename kind of the type name node.
		///
		/// @return Type name kind of the type name node.
		SLAKE_FORCEINLINE TypeNameKind get_typename_kind() const noexcept {
			return tn_kind;
		}

		/// @brief Check if the type name node is with `final` modifier.
		///
		/// @return Whether the type name node is with `final` modifier.
		SLAKE_FORCEINLINE bool is_final() const noexcept {
			return _is_final;
		}

		/// @brief Set if the type name node is with `final` modifier.
		///
		/// @param b Whether the type name node will be set to be with `final` modifier.
		SLAKE_FORCEINLINE void set_final(bool b) noexcept {
			_is_final = b;
		}

		/// @brief Check if the type name node is with `local` modifier.
		///
		/// @return Whether the type name node is with `local` modifier.
		SLAKE_FORCEINLINE bool is_local() const noexcept {
			return _is_local;
		}

		/// @brief Set if the type name node is with `local` modifier.
		///
		/// @param b Whether the type name node is with `local` modifier.
		SLAKE_FORCEINLINE void set_local(bool b) noexcept {
			_is_local = b;
		}

		/// @brief Check if the type name node is nullable.
		///
		/// @return Whether the type name node is nullable.
		SLAKE_FORCEINLINE bool is_nullable() const noexcept {
			return _is_nullable;
		}

		/// @brief Set if the type name node is nullable.
		///
		/// @param b Whether the type name node will be set to be nullable.
		SLAKE_FORCEINLINE void set_nullable(bool b) noexcept {
			_is_nullable = b;
		}

		/// @brief Check if the type name node is explicitly marked as `final` in the source.
		///
		/// @return Whether the type name node is explicitly marked as `final` in the source.
		SLAKE_FORCEINLINE bool is_explicit_final() const noexcept {
			return idx_final_token != SIZE_MAX;
		}

		/// @brief Check if the type name node is explicitly marked as `local` in the source.
		///
		/// @return Whether the type name node is explicitly marked as `local in the source.
		SLAKE_FORCEINLINE bool is_explicit_local() const noexcept {
			return idx_local_token != SIZE_MAX;
		}

		/// @brief Check if the type name node is explicitly marked as nullable in the source.
		///
		/// @return Whether the type name node is explicitly marked as nullable in the source.
		SLAKE_FORCEINLINE bool is_explicit_nullable() const noexcept {
			return idx_nullable_token != SIZE_MAX;
		}
	};
}

#endif
