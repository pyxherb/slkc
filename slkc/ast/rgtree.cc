#include "rgtree.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API GreenNode::GreenNode(Global *global)
	: _global(global),
	  children(global->get_allocator()),
	  node_kind(0) {
	assert(is_token_node_kind(node_kind));
}

SLKC_API GreenNode::~GreenNode() {
}

SLKC_API GreenNodeOperationResult GreenNode::compute_text_width_shallow() noexcept {
	text_width = 0;

	for (const auto &i : children) {
		if (auto t = std::get_if<TokenPtr>(&i); t) {
			text_width += (*t).get()->source_text.get().size();
		} else {
			auto p = std::get_if<GreenNodePtr>(&i);
			auto pinned = p->pin();

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

		if (cur_frame.cur_index >= cur_frame.parent->children.size()) {
			TextWidth total_width = cur_frame.total_width;
			cur_frame.parent->text_width = total_width;
			frames.pop_back();
			if (frames.size())
				frames.back().total_width += total_width;
			continue;
		}

		auto &child = cur_frame.parent->children[cur_frame.cur_index];

		if (auto t = std::get_if<TokenPtr>(&child); t) {
			size_t total_width = (*t)->source_text.get().size();
			frames.back().total_width += total_width;
		} else {
			auto pinned = std::get_if<GreenNodePtr>(&child)->pin();

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
		}
		++cur_frame.cur_index;
	}

	return GreenNodeOperationResult::Success;
}

SLKC_API RedNodeChildIndices::~RedNodeChildIndices() {
}

SLKC_API bool RedNodeChildIndices::index_children(const RedNodePtr &node) noexcept {
	_children_index.clear();

	for (size_t i = 0; i < node->children.size(); ++i) {
		auto &child = node->children[i];

		TokenKind id;
		if (child->is_token_facade())
			id = child->as_token()->token_id;
		else
			id = child->as_green_node()->node_kind;

		if (_children_index.contains(id)) {
			if (!_children_index.insert(+child->as_token()->token_id, peff::DynArray<size_t>(_self_allocator.get())))
				return false;
		}
		if (!_children_index.at(id).push_back(+i))
			return false;
	}

	return true;
}

SLKC_API RedNode::RedNode(peff::Alloc *allocator) : children(allocator) {
}

SLKC_API RedNodePtr ast::build_red_root_node(peff::Alloc *allocator, const GreenNodePin &green_node) {
	RedNodePtr red_root = peff::make_shared<RedNode>(allocator, allocator);

	if (!red_root->children.resize(green_node->children.size()))
		return {};
	red_root->offset = 0;
	red_root->green_node_or_token = green_node;

	return red_root;
}

SLAKE_API GreenNodeOperationResult RedNode::build_child(peff::Alloc *allocator, size_t index) {
	auto g = std::get_if<GreenNodePin>(&green_node_or_token);
	assert(g);
	assert(children.size() == (*g)->children.size());
	if (index)
		assert(children[index - 1]);

	RedNodePtr red_child = peff::make_shared<RedNode>(allocator, allocator);

	auto &child = (*g)->children[index];

	if (auto p = std::get_if<GreenNodePtr>(&child); p) {
		auto pinned = p->pin();
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

		red_child->green_node_or_token = pinned;
	} else {
		red_child->green_node_or_token = *std::get_if<TokenPtr>(&child);
	}

	if (index) {
		if (auto g = std::get_if<GreenNodePin>(&children[index - 1]->green_node_or_token); g)
			red_child->offset = children[index - 1]->offset + (*g)->text_width;
		else
			red_child->offset = children[index - 1]->offset + (*std::get_if<TokenPtr>(&children[index - 1]->green_node_or_token))->source_text.get().size();
	} else
		red_child->offset = this->offset;
	red_child->parent = shared_from_this();
	red_child->parent_index = index;

	this->children[index] = red_child;

	return GreenNodeOperationResult::Success;
}

SLAKE_API GreenNodeOperationResult RedNode::build_children(peff::Alloc *allocator) {
	for (size_t j = 0; j <= children.size(); ++j) {
		GreenNodeOperationResult result = build_child(allocator, j);
		if (result != GreenNodeOperationResult::Success)
			return result;
	}

	return GreenNodeOperationResult::Success;
}

SLAKE_API peff::Result<RedNodePtr, GreenNodeOperationResult> RedNode::get_child_node(peff::Alloc *allocator, size_t index) noexcept {
	size_t i = index + 1;
	while (i) {
		if (children[i - 1])
			break;
		--i;
	}

	for (size_t j = i - 1; j <= index; ++j) {
		GreenNodeOperationResult result = build_child(allocator, j);
		if (result != GreenNodeOperationResult::Success)
			return result;
	}

	return RedNodePtr(children[index]);
}

SLKC_API GreenNodeDumpContext::GreenNodeDumpContext(
	Global *global,
	peff::Alloc *allocator,
	wandjson::ObjectValue *root_value)
	: global(global),
	  root_value(root_value),
	  task_list(allocator),
	  allocator(allocator) {
}

SLKC_API DumpResult GreenNodeDumpContext::push_task(wandjson::ObjectValue *dest, GreenNodeIndex src, bool deep) noexcept {
	if (!task_list.push_back({ src, dest, deep }))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API DumpResult ast::dump_source_token(GreenNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, const TokenPtr &token) noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
		return DumpResult::OutOfMemory;
	wandjson::ObjectValue *token_object = static_cast<wandjson::ObjectValue *>(v.get());
	if (!target_object->insert("source_token", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::StringValue::alloc(dump_context.get_allocator(), token->source_text.get()))))
		return DumpResult::OutOfMemory;
	if (!token_object->insert("source_text", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), token->token_id))))
		return DumpResult::OutOfMemory;
	if (!token_object->insert("token_id", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API DumpResult ast::dump_green_node(GreenNodeDumpContext &dump_context, wandjson::ObjectValue *target_object, const GreenNodePin &node, bool deep) noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), node->node_kind))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("node_kind", v.release()))
		return DumpResult::OutOfMemory;

	if (!std::get_if<std::monostate>(&node->exdata)) {
		if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ObjectValue *exdata_object = static_cast<wandjson::ObjectValue *>(v.get());
		if (!target_object->insert("exdata", v.release()))
			return DumpResult::OutOfMemory;
		if (auto exdata = std::get_if<TypeNameGreenNodeExData>(&node->exdata); exdata) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(exdata->type_name_kind)))))
				return DumpResult::OutOfMemory;
			if (!exdata_object->insert("type_name_kind", v.release()))
				return DumpResult::OutOfMemory;
		} else if (auto exdata = std::get_if<ExprGreenNodeExData>(&node->exdata); exdata) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(exdata->expr_kind)))))
				return DumpResult::OutOfMemory;
			if (!exdata_object->insert("expr_kind", v.release()))
				return DumpResult::OutOfMemory;

			switch (exdata->expr_kind) {
				case slkc::ast::GreenNodeExprKind::Unary:
					if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(exdata->unary_expr_op)))))
						return DumpResult::OutOfMemory;
					if (!exdata_object->insert("unary_expr_op", v.release()))
						return DumpResult::OutOfMemory;
					break;
				case slkc::ast::GreenNodeExprKind::Binary:
					if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(exdata->binary_expr_op)))))
						return DumpResult::OutOfMemory;
					if (!exdata_object->insert("binary_expr_op", v.release()))
						return DumpResult::OutOfMemory;
					break;
				default:
					break;
			}
		} else if (auto exdata = std::get_if<StmtGreenNodeExData>(&node->exdata); exdata) {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(exdata->stmt_kind)))))
				return DumpResult::OutOfMemory;
			if (!exdata_object->insert("stmt_kind", v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	if (node->children.size()) {
		if (!(v = decltype(v)(wandjson::ArrayValue::alloc(dump_context.get_allocator()))))
			return DumpResult::OutOfMemory;
		wandjson::ArrayValue *children_array = static_cast<wandjson::ArrayValue *>(v.get());
		if (!target_object->insert("children", v.release()))
			return DumpResult::OutOfMemory;

		if (deep) {
			for (size_t i = 0; i < node->children.size(); ++i) {
				auto &child = node->children[i];

				if (auto t = std::get_if<TokenPtr>(&child); t) {
					if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
						return DumpResult::OutOfMemory;
					wandjson::ObjectValue *node_obj = static_cast<wandjson::ObjectValue *>(v.get());
					if (!children_array->push_back(v.release()))
						return DumpResult::OutOfMemory;
					SLKC_RETURN_IF_DUMP_FAILED(dump_source_token(dump_context, node_obj, *t));
				} else {
					if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
						return DumpResult::OutOfMemory;
					wandjson::ObjectValue *node_obj = static_cast<wandjson::ObjectValue *>(v.get());
					if (!children_array->push_back(v.release()))
						return DumpResult::OutOfMemory;
					SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(node_obj, std::get_if<GreenNodePtr>(&child)->get_index(), deep));
				}
			}
		} else {
			for (size_t i = 0; i < node->children.size(); ++i) {
				auto &child = node->children[i];

				if (auto t = std::get_if<TokenPtr>(&child); t) {
					if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
						return DumpResult::OutOfMemory;
					wandjson::ObjectValue *node_obj = static_cast<wandjson::ObjectValue *>(v.get());
					if (!children_array->push_back(v.release()))
						return DumpResult::OutOfMemory;
					SLKC_RETURN_IF_DUMP_FAILED(dump_source_token(dump_context, node_obj, *t));
				} else {
					if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), std::get_if<GreenNodePtr>(&child)->get_index()))))
						return DumpResult::OutOfMemory;
					if (!children_array->push_back(v.release()))
						return DumpResult::OutOfMemory;
				}
			}
		}
	}

	return DumpResult::Ok;
}
