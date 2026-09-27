#ifndef _SLKC_AST_NODEDEFS_TYPE_BASE_H_
#define _SLKC_AST_NODEDEFS_TYPE_BASE_H_

#include "../astnode.h"

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

		enum class TypeNameNullability : uint8_t {
			Unspecified = 0,
			Nullable,
			NonNullable,
		};

		enum class TypeNameShareability : uint8_t {
			Unspecified = 0,
			Multi,
			Restrict,
			Synchronized,
		};

		class TypeNameNode : public AstNode {
		protected:
			const TypeNameKind _tn_kind;

			bool _is_const : 1;
			bool _is_final : 1;
			TypeNameNullability _nullability : 2;
			TypeNameShareability _shareability : 2;

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			SLKC_API TypeNameNode(Global *global, TypeNameKind kind);
			SLKC_API TypeNameNode(const TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~TypeNameNode();

			SLAKE_FORCEINLINE TypeNameKind get_tn_kind() const noexcept {
				return _tn_kind;
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

			SLAKE_FORCEINLINE TypeNameNullability get_nullability() const noexcept {
				return _nullability;
			}

			SLAKE_FORCEINLINE void set_nullability(TypeNameNullability nullability) noexcept {
				_nullability = nullability;
			}

			SLAKE_FORCEINLINE TypeNameShareability get_shareability() const noexcept {
				return _shareability;
			}

			SLAKE_FORCEINLINE void set_shareability(TypeNameShareability shareability) noexcept {
				_shareability = shareability;
			}
		};
	}
}

#endif
