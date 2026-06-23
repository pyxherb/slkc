#include "typename_base.h"

using namespace slkc;

SLKC_API TypeNameNode::TypeNameNode(TypeNameKind tn_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::TypeName, self_allocator, document), tn_kind(tn_kind) {
}

SLKC_API TypeNameNode::TypeNameNode(const TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context)
	: AstNode(rhs, self_allocator, context),
	  tn_kind(rhs.tn_kind),
	  _is_final(rhs._is_final),
	  _is_local(rhs._is_local),
	  _is_nullable(rhs._is_nullable),
	  idx_final_token(rhs.idx_final_token),
	  idx_local_token(rhs.idx_local_token) {
}

SLKC_API TypeNameNode::~TypeNameNode() {
}
