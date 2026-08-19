#include "rgtree.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API RGNode::RGNode(Global *global) : _global(global), source_token(nullptr), children(global->get_allocator()), node_kind(0u) {
	assert(is_token_node_kind(node_kind));
}

SLKC_API RGNode::~RGNode() {
}

SLKC_API bool RGNode::index_children() noexcept {
	return true;
}
