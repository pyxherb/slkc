#include "rgtree.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API GreenNode::GreenNode(Global *global)
	: _global(global),
	  source_token(nullptr),
	  children(global->get_allocator()),
	  node_kind(0) {
	assert(is_token_node_kind(node_kind));
}

SLKC_API GreenNode::~GreenNode() {
}

SLKC_API peff::Option<PinFailReason> GreenNode::compute_text_width_shallow() noexcept {
	text_width = 0;

	for(const auto &i : children) {
		auto pinned = i.pin();

		if(pinned.is_fail())
			return pinned.get_fail_reason();

		text_width += pinned->text_width;
	}

	return peff::NULLOPT;
}

SLKC_API GreenNodeChildrenIndex::~GreenNodeChildrenIndex() {
}

SLKC_API GreenNodeIndexingResult GreenNodeChildrenIndex::index_node(const GreenNodePin &node) noexcept {
	children_index.clear();

	for (size_t i = 0 ; i < node->children.size(); ++i) {
		GreenNodePin pinned = node->children[i].pin();

		if (pinned.is_fail()) {
			switch (pinned.get_fail_reason()) {
				case slkc::ast::PinFailReason::IOError:
					return GreenNodeIndexingResult::PinIOError;
				case slkc::ast::PinFailReason::OutOfMemory:
					return GreenNodeIndexingResult::OutOfMemory;
				default:
					break;
			}
			SLAKE_UNREACHABLE();
		}

		if(!children_index.insert(+pinned->node_kind, +i))
			return GreenNodeIndexingResult::OutOfMemory;
	}

	return GreenNodeIndexingResult::Success;
}
