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

SLKC_API GreenNodeOperationResult GreenNode::compute_text_width_shallow() noexcept {
	text_width = 0;

	for (const auto &i : children) {
		auto pinned = i.pin();

		if (pinned.is_fail()) {
			switch (pinned.get_fail_reason()) {
				case slkc::ast::PinFailReason::IOError:
					return GreenNodeOperationResult::PinIOError;
				case slkc::ast::PinFailReason::OutOfMemory:
					return GreenNodeOperationResult::OutOfMemory;
				case slkc::ast::PinFailReason::OutOfNodeIndex:
					return GreenNodeOperationResult::OutOfNodeIndex;
				default:
					SLAKE_UNREACHABLE();
			}
			std::terminate();
		}

		text_width += pinned->text_width;
	}

	return GreenNodeOperationResult::Success;
}

struct GreenNodeTextWidthComputingFrame {
	GreenNodePin parent;
	size_t cur_index = 0;
	size_t total_width = 0;
};

SLKC_API GreenNodeOperationResult ast::compute_green_node_text_width_deep(GreenNodePin root, peff::Alloc *allocator, bool forced_update) noexcept {
	peff::List<GreenNodeTextWidthComputingFrame> frames(allocator);

	if (!frames.push_back({ root, 0, 0 }))
		return GreenNodeOperationResult::OutOfMemory;

	while (frames.size()) {
		auto &cur_frame = frames.back();

		if ((!forced_update) && cur_frame.parent->text_width) {
			frames.back().total_width += cur_frame.parent->text_width;
			frames.pop_back();
			continue;
		}

		if (cur_frame.parent->source_token) {
			TextWidth total_width = cur_frame.parent->source_token->source_text.get().size();
			cur_frame.parent->text_width = total_width;
			frames.pop_back();
			if (frames.size())
				frames.back().total_width += total_width;
			continue;
		} else {
			if (cur_frame.cur_index >= cur_frame.parent->children.size()) {
				TextWidth total_width = cur_frame.total_width;
				cur_frame.parent->text_width = total_width;
				frames.pop_back();
				if (frames.size())
					frames.back().total_width += total_width;
				continue;
			}

			auto pinned = cur_frame.parent->children[cur_frame.cur_index].pin();

			if (pinned.is_fail()) {
				switch (pinned.get_fail_reason()) {
					case slkc::ast::PinFailReason::IOError:
						return GreenNodeOperationResult::PinIOError;
					case slkc::ast::PinFailReason::OutOfMemory:
						return GreenNodeOperationResult::OutOfMemory;
					case slkc::ast::PinFailReason::OutOfNodeIndex:
						return GreenNodeOperationResult::OutOfNodeIndex;
					default:
						SLAKE_UNREACHABLE();
				}
				std::terminate();
			}

			if (!frames.push_back({ pinned, 0, 0 }))
				return GreenNodeOperationResult::OutOfMemory;

			++cur_frame.cur_index;
		}
	}

	return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeChildrenIndex::~GreenNodeChildrenIndex() {
}

SLKC_API GreenNodeOperationResult GreenNodeChildrenIndex::index_node(const GreenNodePin &node) noexcept {
	children_index.clear();

	for (size_t i = 0; i < node->children.size(); ++i) {
		GreenNodePin pinned = node->children[i].pin();

		if (pinned.is_fail()) {
			switch (pinned.get_fail_reason()) {
				case slkc::ast::PinFailReason::IOError:
					return GreenNodeOperationResult::PinIOError;
				case slkc::ast::PinFailReason::OutOfMemory:
					return GreenNodeOperationResult::OutOfMemory;
				default:
					break;
			}
			SLAKE_UNREACHABLE();
		}

		if (!children_index.insert(+pinned->node_kind, +i))
			return GreenNodeOperationResult::OutOfMemory;
	}

	return GreenNodeOperationResult::Success;
}
