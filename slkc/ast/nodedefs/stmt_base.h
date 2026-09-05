#ifndef _SLKC_AST_NODEDEFS_STMT_BASE_H_
#define _SLKC_AST_NODEDEFS_STMT_BASE_H_

#include "idref.h"

namespace slkc {
	namespace ast {
		enum class StmtKind : uint8_t {
			Expr = 0,  // Expression
			Let,	   // Let binding
			For,	   // For
			ForEach,   // For each
			While,	   // While
			DoWhile,   // Do while
			If,		   // If
			Switch,	   // Switch
			Block,	   // Code block
		};

		class StmtNode : public AstNode {
		protected:
			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		private:
			const StmtKind _stmt_kind;
			bool _is_bad;

		public:
			SLKC_API StmtNode(StmtKind stmt_kind, Global *global);
			SLKC_API StmtNode(const StmtNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~StmtNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();

			SLAKE_FORCEINLINE StmtKind get_stmt_kind() const noexcept {
				return _stmt_kind;
			}

			SLAKE_FORCEINLINE bool is_bad() const noexcept {
				return _is_bad;
			}

			SLAKE_FORCEINLINE void set_bad(bool bad) noexcept {
				_is_bad = bad;
			}
		};
	}
}

#endif
