#ifndef _SLKC_AST_NODEDEFS_TYPE_H_
#define _SLKC_AST_NODEDEFS_TYPE_H_

#include "type_base.h"
#include "idref.h"

namespace slkc {
	namespace ast {
		class CustomTypeDefNode : public Node {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

			friend Global;

		public:
			OwnedIdRef referred_name;

			SLKC_API CustomTypeDefNode(Global *global);
			SLKC_API CustomTypeDefNode(const CustomTypeDefNode &other, DuplicationContext &context, peff::Option<DuplicationResult> &result_out);
			SLKC_API virtual ~CustomTypeDefNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ArrayTypeDefNode : public Node {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

			friend Global;

		public:
			TypeName element_type;

			SLKC_API ArrayTypeDefNode(Global *global);
			SLKC_API ArrayTypeDefNode(const ArrayTypeDefNode &other, DuplicationContext &context, peff::Option<DuplicationResult> &result_out);
			SLKC_API virtual ~ArrayTypeDefNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
