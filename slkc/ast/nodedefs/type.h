#ifndef _SLKC_AST_NODEDEFS_TYPE_H_
#define _SLKC_AST_NODEDEFS_TYPE_H_

#include "type_base.h"
#include "idref.h"

namespace slkc {
	namespace ast {
		class I8TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API I8TypeNameNode(Global *global);
			SLKC_API I8TypeNameNode(const I8TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~I8TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I16TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API I16TypeNameNode(Global *global);
			SLKC_API I16TypeNameNode(const I16TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~I16TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I32TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API I32TypeNameNode(Global *global);
			SLKC_API I32TypeNameNode(const I32TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~I32TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class I64TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API I64TypeNameNode(Global *global);
			SLKC_API I64TypeNameNode(const I64TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~I64TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ISizeTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API ISizeTypeNameNode(Global *global);
			SLKC_API ISizeTypeNameNode(const ISizeTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~ISizeTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U8TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API U8TypeNameNode(Global *global);
			SLKC_API U8TypeNameNode(const U8TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~U8TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U16TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API U16TypeNameNode(Global *global);
			SLKC_API U16TypeNameNode(const U16TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~U16TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U32TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API U32TypeNameNode(Global *global);
			SLKC_API U32TypeNameNode(const U32TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~U32TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class U64TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API U64TypeNameNode(Global *global);
			SLKC_API U64TypeNameNode(const U64TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~U64TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class USizeTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API USizeTypeNameNode(Global *global);
			SLKC_API USizeTypeNameNode(const USizeTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~USizeTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class F32TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API F32TypeNameNode(Global *global);
			SLKC_API F32TypeNameNode(const F32TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~F32TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class F64TypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API F64TypeNameNode(Global *global);
			SLKC_API F64TypeNameNode(const F64TypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~F64TypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class StringTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API StringTypeNameNode(Global *global);
			SLKC_API StringTypeNameNode(const StringTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~StringTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class BoolTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API BoolTypeNameNode(Global *global);
			SLKC_API BoolTypeNameNode(const BoolTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~BoolTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class VoidTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			
			SLKC_API VoidTypeNameNode(Global *global);
			SLKC_API VoidTypeNameNode(const VoidTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~VoidTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ObjectTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API ObjectTypeNameNode(Global *global);
			SLKC_API ObjectTypeNameNode(const ObjectTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~ObjectTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class AnyTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API AnyTypeNameNode(Global *global);
			SLKC_API AnyTypeNameNode(const AnyTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~AnyTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class NeverTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

		public:
			SLKC_API NeverTypeNameNode(Global *global);
			SLKC_API NeverTypeNameNode(const NeverTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index);
			SLKC_API virtual ~NeverTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class CustomTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			OwnedIdRef referred_name;

			SLKC_API CustomTypeNameNode(Global *global);
			SLKC_API CustomTypeNameNode(const CustomTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~CustomTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ArrayTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			AstNodePtr<TypeNameNode> element_type;

			SLKC_API ArrayTypeNameNode(Global *global);
			SLKC_API ArrayTypeNameNode(const ArrayTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ArrayTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class RefTypeNameNode final : public TypeNameNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(AstNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			AstNodePtr<TypeNameNode> element_type;

			SLKC_API RefTypeNameNode(Global *global);
			SLKC_API RefTypeNameNode(const RefTypeNameNode &other, AstNodeDuplicationContext &context, AstNodeIndex node_index, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~RefTypeNameNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
