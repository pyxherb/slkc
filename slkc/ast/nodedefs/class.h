#ifndef _SLKC_AST_NODEDEFS_CLASS_H_
#define _SLKC_AST_NODEDEFS_CLASS_H_

#include "member.h"
#include <peff/containers/bitarray.h>

namespace slkc {
	namespace ast {
		enum class GenericVariance : uint8_t {
			None = 0,
			In,
			Out
		};

		struct GenericConstraint {
			peff::Option<TypeName> inherited_type;
			peff::DynArray<ImplementItem> implemented_types;

			GenericVariance generic_variance;

			TokenIndex sti_inherit_left_paren = INVALID_TOKEN_INDEX,
				   sti_inherit_right_paren = INVALID_TOKEN_INDEX,
				   sti_implement_colon = INVALID_TOKEN_INDEX,
				   sti_generic_variance_indicator = INVALID_TOKEN_INDEX;
			peff::DynArray<TokenIndex> sti_implement_item_separator;

			SLKC_API GenericConstraint(Global *global);
			SLKC_API ~GenericConstraint();

			SLKC_API GenericConstraint(const GenericConstraint &) = delete;
			SLKC_API GenericConstraint(GenericConstraint &&) noexcept = default;

			SLKC_API peff::Result<GenericConstraint, DuplicationError> deep_duplicate(DuplicationContext &duplication_context) noexcept;
		};

		class GenericParamNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			peff::Option<GenericConstraint> generic_constraint;
			TokenIndex sti_name = INVALID_TOKEN_INDEX;

			SLKC_API GenericParamNode(Global *global);
			SLKC_API GenericParamNode(const GenericParamNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~GenericParamNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ClassNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_class_keyword = INVALID_TOKEN_INDEX;

			SLKC_API ClassNode(Global *global);
			SLKC_API ClassNode(const ClassNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ClassNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class InterfaceNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_interface_keyword = INVALID_TOKEN_INDEX;

			SLKC_API InterfaceNode(Global *global);
			SLKC_API InterfaceNode(const InterfaceNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~InterfaceNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ExceptNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_except_keyword = INVALID_TOKEN_INDEX;

			SLKC_API ExceptNode(Global *global);
			SLKC_API ExceptNode(const ExceptNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ExceptNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class TraitNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_trait_keyword = INVALID_TOKEN_INDEX;

			SLKC_API TraitNode(Global *global);
			SLKC_API TraitNode(const TraitNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~TraitNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class StructNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_struct_keyword = INVALID_TOKEN_INDEX;

			SLKC_API StructNode(Global *global);
			SLKC_API StructNode(const StructNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~StructNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ConstEnumNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_const_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_enum_keyword = INVALID_TOKEN_INDEX;

			SLKC_API ConstEnumNode(Global *global);
			SLKC_API ConstEnumNode(const ConstEnumNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ConstEnumNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class ScopedEnumNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_enum_keyword = INVALID_TOKEN_INDEX;

			SLKC_API ScopedEnumNode(Global *global);
			SLKC_API ScopedEnumNode(const ScopedEnumNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~ScopedEnumNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};

		class UnionEnumNode : public MemberNode {
		protected:
			SLKC_SIMPLE_AST_DUPLICATE_FN_DECL();

			[[nodiscard]] SLKC_API virtual DumpResult do_dump(DumpContext &dump_context, wandjson::ObjectValue *target_object, bool deep_dump) const noexcept override;

		public:
			TokenIndex sti_enum_keyword = INVALID_TOKEN_INDEX;
			TokenIndex sti_union_keyword = INVALID_TOKEN_INDEX;

			SLKC_API UnionEnumNode(Global *global);
			SLKC_API UnionEnumNode(const UnionEnumNode &other, DuplicationContext &context, peff::Option<DuplicationError> &error_out);
			SLKC_API virtual ~UnionEnumNode();

			SLKC_SIMPLE_AST_DEALLOC_FN_DECL();
		};
	}
}

#endif
