#include "rgtree.h"
#include <peff/containers/bitarray.h>

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

SLKC_API GreenNodeOperationResult ast::pin_fail_reason_to_green_node_operation_result(PinFailReason reason) {
	switch (reason) {
		case slkc::ast::PinFailReason::IOError:
			return GreenNodeOperationResult::PinIOError;
		case slkc::ast::PinFailReason::OutOfMemory:
			return GreenNodeOperationResult::OutOfMemory;
		case slkc::ast::PinFailReason::OutOfNodeIndex:
			return GreenNodeOperationResult::OutOfNodeIndex;
		default:
			std::terminate();
	}
}

SLKC_API GreenNodeOperationResult GreenNode::compute_text_width_shallow() noexcept {
	text_width = 0;

	for (const auto &i : children) {
		if (auto t = std::get_if<TokenPtr>(&i); t) {
			text_width += (*t).get()->source_text.get_view().size();
		} else {
			auto p = std::get_if<GreenNodePtr>(&i);
			auto pinned = p->pin();

			if (pinned.is_fail()) {
				return pin_fail_reason_to_green_node_operation_result(pinned.get_fail_reason());
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
			size_t total_width = (*t)->source_text.get_view().size();
			frames.back().total_width += total_width;
		} else {
			auto pinned = std::get_if<GreenNodePtr>(&child)->pin();

			if (pinned.is_fail()) {
				return pin_fail_reason_to_green_node_operation_result(pinned.get_fail_reason());
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
			return pin_fail_reason_to_green_node_operation_result(pinned.get_fail_reason());
		}

		red_child->green_node_or_token = pinned;
	} else {
		red_child->green_node_or_token = *std::get_if<TokenPtr>(&child);
	}

	if (index) {
		if (auto g = std::get_if<GreenNodePin>(&children[index - 1]->green_node_or_token); g)
			red_child->offset = children[index - 1]->offset + (*g)->text_width;
		else
			red_child->offset = children[index - 1]->offset + (*std::get_if<TokenPtr>(&children[index - 1]->green_node_or_token))->source_text.get_view().size();
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

	if (!(v = decltype(v)(wandjson::StringValue::alloc(dump_context.get_allocator(), token->source_text.get_view()))))
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

SLKC_API std::strong_ordering GreenNodeDiff::operator<=>(const GreenNodeDiff &rhs) const noexcept {
	if (auto result = is_dest_path <=> rhs.is_dest_path; result != 0)
		return result;
	if (auto result = path.size() <=> rhs.path.size(); result != 0)
		return result;
	for (size_t i = 0; i < path.size(); ++i) {
		if (auto result = path[i] <=> rhs.path[i]; result != 0)
			return result;
	}
	return std::strong_ordering::equivalent;
}

SLAKE_API GreenNodeOperationResult GreenNodeDiffCoroutine::resume(GreenNodeDiffCoroutineScheduler *scheduler) {
	if (!coro_handle)
		return GreenNodeOperationResult::OutOfMemory;

	coro_handle.resume();

	while (scheduler->task_list.size()) {
		auto h = scheduler->task_list.back();
		scheduler->task_list.pop_back();
		if (!h.done())
			h.resume();
		if (coro_handle.promise().result != GreenNodeOperationResult::Success)
			return coro_handle.promise().result;
	}

	if (coro_handle.promise().result != GreenNodeOperationResult::Success)
		return coro_handle.promise().result;
	if (!coro_handle.done())
		std::terminate();

	return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeDiffCoroutine::Awaitable::Awaitable(
	GreenNodeDiffCoroutine &co,
	GreenNodeDiffCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLKC_API bool GreenNodeDiffCoroutine::Awaitable::await_ready() {
	return false;
}

SLKC_API void GreenNodeDiffCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = GreenNodeOperationResult::OutOfMemory;
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = GreenNodeOperationResult::OutOfMemory;
		return;
	}
}

SLKC_API std::strong_ordering GreenNodeDiffCachePair::operator<=>(const GreenNodeDiffCachePair &rhs) const noexcept {
	if (auto result = lhs_node <=> rhs.lhs_node; result != std::strong_ordering::equivalent) {
		assert(lhs_node != rhs.lhs_node);
		return result;
	}
	if (auto result = rhs_node <=> rhs.rhs_node; result != std::strong_ordering::equivalent) {
		assert(rhs_node != rhs.rhs_node);
		return result;
	}
	return std::strong_ordering::equivalent;
}

SLKC_API GreenNodeOperationResult GreenNodeDiffCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result != GreenNodeOperationResult::Success)
			return handle.promise().result;
		return GreenNodeOperationResult::Success;
	}
	return GreenNodeOperationResult::OutOfMemory;
}

SLKC_API GreenNodeDiffCoroutine::Awaitable GreenNodeDiffCoroutine::operator()(GreenNodeDiffCoroutineScheduler *scheduler) {
	return Awaitable(*this, scheduler, coro_handle);
}

SLKC_API GreenNodeDiffCoroutineScheduler::GreenNodeDiffCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

SLKC_API GreenNodeDiffCoroutine ast::_do_simple_green_tree_diff(
	peff::Alloc *allocator,
	GreenNodeDiffCoroutineScheduler &scheduler,
	const GreenNodePin &lhs,
	const GreenNodePin &rhs,
	std::span<size_t> lhs_path_base,
	GreenNodeDiffSet *diff_set_out,
	GreenNodeDiffCache &diff_caches,
	bool &is_same_out) {
	assert(lhs);
	assert(rhs);
	if (!diff_set_out) {
		// Disable caches for debugging, the cache mechanism is buggy right now.
		/* if (auto it = diff_caches.find({ GreenNodePin(lhs), GreenNodePin(rhs) }); it != diff_caches.end()) {
			is_same_out = it.value();
			co_return GreenNodeOperationResult::Success;
		}*/
	}
	assert(lhs->node_kind == rhs->node_kind);

	peff::BitArray lhs_occupation_set(allocator);
	peff::BitArray rhs_occupation_set(allocator);
	peff::Map<size_t, size_t> lhs_to_rhs_move_map(allocator);

	if (!lhs_occupation_set.resize(lhs->children.size()))
		co_return GreenNodeOperationResult::OutOfMemory;
	if (!rhs_occupation_set.resize(rhs->children.size()))
		co_return GreenNodeOperationResult::OutOfMemory;

	// Build extended path view array for difference tracing.
	peff::DynArray<size_t> extended_path(allocator);
	if (!extended_path.build(lhs_path_base))
		co_return GreenNodeOperationResult::OutOfMemory;
	if (!extended_path.push_back(SIZE_MAX))
		co_return GreenNodeOperationResult::OutOfMemory;

	bool is_all_same = true;
	for (size_t i = 0; i < lhs->children.size(); ++i) {
		extended_path.back() = i;

		if (auto it = std::get_if<GreenNodePtr>(&lhs->children[i]); it) {
			auto pinned_lhs = it->pin();

			if (pinned_lhs.is_fail())
				co_return pin_fail_reason_to_green_node_operation_result(pinned_lhs.get_fail_reason());

			for (size_t j = 0; j < rhs->children.size(); ++j) {
				if (rhs_occupation_set.get_bit(j))
					continue;
				if (auto r = std::get_if<GreenNodePtr>(&rhs->children[j]); r) {
					auto pinned_rhs = r->pin();

					if (pinned_rhs.is_fail())
						co_return pin_fail_reason_to_green_node_operation_result(pinned_rhs.get_fail_reason());

					if (pinned_lhs->node_kind == pinned_rhs->node_kind) {
						bool same = true;

						// Check if the subtrees are the same.
						// This does not write to the difference set.
						if (auto result = co_await _do_simple_green_tree_diff(allocator, scheduler, pinned_lhs, pinned_rhs, extended_path, nullptr, diff_caches, same)(&scheduler); result != GreenNodeOperationResult::Success)
							co_return result;

						if (same) {
							assert(!lhs_to_rhs_move_map.contains(i));
							if (!lhs_to_rhs_move_map.insert(+i, +j))
								co_return GreenNodeOperationResult::OutOfMemory;
							lhs_occupation_set.set_bit(i);
							rhs_occupation_set.set_bit(j);

							if (i != j)
								is_all_same = false;

							// We cannot determine if the first element is moved, if first element is moved backward,
							// the latter elements will be marked as moved, so there is no need to mark it.
							if (i) {
								bool found_in_order_member = false, found_sibling = false;
								// If the node is relatively kept from original order, DO NOT generate a difference.
								for (size_t k = i; k; --k) {
									if (auto it = lhs_to_rhs_move_map.find(k - 1); it != lhs_to_rhs_move_map.end()) {
										assert(k - 1 != i);
										found_sibling = true;
										if (j > it.value()) {
											// Once we found a member in order, it shows us that it is in original order.
											found_in_order_member = true;
										}
										break;
									}
								}

								// The node is moved, generate a difference.
								if (diff_set_out) {
									if (found_in_order_member) {
										// Found members in order means the relative position is not changed.
									} else {
										GreenNodeDiff diff(allocator);

										diff.kind = GreenNodeDiffKind::Moved;
										diff.exdata.moved.moved_to_index = j;
										if (!diff.path.build(extended_path))
											co_return GreenNodeOperationResult::OutOfMemory;
										if (!diff_set_out->insert(std::move(diff)))
											co_return GreenNodeOperationResult::OutOfMemory;
									}
								}
							}

							goto node_success;
						} else {
							if (i == j)
								is_all_same = false;
						}
					} else {
						if (i == j)
							is_all_same = false;
					}
				} else {
					if (i == j)
						is_all_same = false;
				}
			}

			is_all_same = false;

		node_success:;
		} else {
			TokenPtr token = *std::get_if<TokenPtr>(&lhs->children[i]);

			for (size_t j = 0; j < rhs->children.size(); ++j) {
				if (rhs_occupation_set.get_bit(j))
					continue;
				if (auto r = std::get_if<TokenPtr>(&rhs->children[j]); r) {
					if ((token->token_id == r->get()->token_id) && (token->source_text.get_view() == r->get()->source_text.get_view())) {
						assert(!lhs_to_rhs_move_map.contains(i));
						if (!lhs_to_rhs_move_map.insert(+i, +j))
							co_return GreenNodeOperationResult::OutOfMemory;
						lhs_occupation_set.set_bit(i);
						rhs_occupation_set.set_bit(j);

						if (i != j)
							is_all_same = false;

						// Just like above.
						if (i) {
							bool found_in_order_member = false, found_sibling = false;
							for (size_t k = i; k; --k) {
								if (auto it = lhs_to_rhs_move_map.find(k - 1); it != lhs_to_rhs_move_map.end()) {
									assert(k - 1 != i);
									found_sibling = true;
									auto prev = it.value();
									if (j > prev) {
										// Once we found a member in order, it shows us that it is in original order.
										found_in_order_member = true;
									}
									break;
								}
							}

							if (diff_set_out) {
								if (found_in_order_member) {
									// Just like above, relative position was not changed.
								} else {
									GreenNodeDiff diff(allocator);

									diff.kind = GreenNodeDiffKind::Moved;
									diff.exdata.moved.moved_to_index = j;
									if (!diff.path.build(extended_path))
										co_return GreenNodeOperationResult::OutOfMemory;
									if (!diff_set_out->insert(std::move(diff)))
										co_return GreenNodeOperationResult::OutOfMemory;
								}
							}
						}

						goto token_success;
					} else {
						if (i == j)
							is_all_same = false;
					}
				} else {
					if (i == j)
						is_all_same = false;
				}
			}

			is_all_same = false;
		token_success:;
		}
	}

	// Scan again to match subnodes with the same kind and generate differences.
	for (size_t i = 0; i < lhs_occupation_set.bit_size(); ++i) {
		if (lhs_occupation_set.get_bit(i))
			continue;
		is_all_same = false;
		extended_path.back() = i;
		if (auto it = std::get_if<GreenNodePtr>(&lhs->children[i]); it) {
			auto pinned_lhs = it->pin();

			if (pinned_lhs.is_fail())
				co_return pin_fail_reason_to_green_node_operation_result(pinned_lhs.get_fail_reason());

			// Find a proper same kind node to compare.
			// If not found, the node should be marked as removed.
			for (size_t j = 0; j < rhs->children.size(); ++j) {
				if (rhs_occupation_set.get_bit(j))
					continue;
				if (auto r = std::get_if<GreenNodePtr>(&rhs->children[j]); r) {
					auto pinned_rhs = r->pin();

					if (pinned_rhs.is_fail())
						co_return pin_fail_reason_to_green_node_operation_result(pinned_rhs.get_fail_reason());

					if (pinned_rhs->node_kind == pinned_lhs->node_kind) {
						bool found_sibling_in_order = false, found_sibling = false;
						for (size_t k = i + 1; k < lhs->children.size(); ++k) {
							if (auto it = lhs_to_rhs_move_map.find(k); it != lhs_to_rhs_move_map.end()) {
								found_sibling = true;
								if (j < it.value()) {
									found_sibling_in_order = true;
								}
								break;
							}
						}

						assert(!lhs_to_rhs_move_map.contains(i));
						lhs_occupation_set.set_bit(i);
						if (!lhs_to_rhs_move_map.insert(+i, +j))
							co_return GreenNodeOperationResult::OutOfMemory;
						rhs_occupation_set.set_bit(j);

						if (found_sibling_in_order) {
							bool same;
							if (auto result = co_await _do_simple_green_tree_diff(allocator, scheduler, pinned_lhs, pinned_rhs, extended_path, diff_set_out, diff_caches, same)(&scheduler); result != GreenNodeOperationResult::Success)
								co_return result;
						} else if (found_sibling) {
							if (diff_set_out) {
								GreenNodeDiff diff(allocator);

								diff.kind = GreenNodeDiffKind::Moved;
								diff.exdata.moved.moved_to_index = j;
								if (!diff.path.build(extended_path))
									co_return GreenNodeOperationResult::OutOfMemory;
								if (!diff_set_out->insert(std::move(diff)))
									co_return GreenNodeOperationResult::OutOfMemory;
							}
						} else {
							// The relative order did not change.
						}
						goto node_matched;
					}
				}
			}

			if (diff_set_out) {
				GreenNodeDiff diff(allocator);
				diff.kind = GreenNodeDiffKind::RemovedFromLhs;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}

		node_matched:;
		} else {
			TokenPtr token = *std::get_if<TokenPtr>(&lhs->children[i]);

			// Just like above, but applies to the token children.
			for (size_t j = 0; j < rhs->children.size(); ++j) {
				if (rhs_occupation_set.get_bit(j))
					continue;
				if (auto r = std::get_if<TokenPtr>(&rhs->children[j]); r) {
					// Choose the token with the same kind to mark out that the token has been updated.
					if (token->token_id == r->get()->token_id) {
						bool found_sibling_in_order = false, found_sibling = false;
						for (size_t k = i + 1; k < lhs->children.size(); ++k) {
							if (auto it = lhs_to_rhs_move_map.find(k); it != lhs_to_rhs_move_map.end()) {
								found_sibling = true;
								if (j < it.value()) {
									found_sibling_in_order = true;
								}
								break;
							}
						}

						assert(!lhs_to_rhs_move_map.contains(i));
						lhs_occupation_set.set_bit(i);
						if (!lhs_to_rhs_move_map.insert(+i, +j))
							co_return GreenNodeOperationResult::OutOfMemory;
						rhs_occupation_set.set_bit(j);

						if (token->source_text.get_view() == r->get()->source_text.get_view()) {
							if (found_sibling_in_order) {
							} else if (found_sibling) {
								if (diff_set_out) {
									GreenNodeDiff diff(allocator);

									diff.kind = GreenNodeDiffKind::Moved;
									diff.exdata.moved.moved_to_index = j;
									if (!diff.path.build(extended_path))
										co_return GreenNodeOperationResult::OutOfMemory;
									if (!diff_set_out->insert(std::move(diff)))
										co_return GreenNodeOperationResult::OutOfMemory;
								}
							} else {
								// The relative order did not change.
							}
						} else {
							if (diff_set_out) {
								GreenNodeDiff diff(allocator);

								diff.kind = GreenNodeDiffKind::ReplacedLhsNode;
								if (!diff.path.build(extended_path))
									co_return GreenNodeOperationResult::OutOfMemory;
								if (!diff_set_out->insert(std::move(diff)))
									co_return GreenNodeOperationResult::OutOfMemory;
							}
						}

						goto token_matched;
					}
				}
			}

			if (diff_set_out) {
				GreenNodeDiff diff(allocator);
				diff.kind = GreenNodeDiffKind::RemovedFromLhs;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}

		token_matched:;
		}
	}

	if (diff_set_out) {
		for (size_t i = 0; i < rhs_occupation_set.bit_size(); ++i) {
			if (!rhs_occupation_set.get_bit(i)) {
				is_all_same = false;
				extended_path.back() = i;

				GreenNodeDiff diff(allocator);

				// Insert a moved difference and wait for further trimming.
				diff.kind = GreenNodeDiffKind::InsertedIntoRhs;
				diff.is_dest_path = true;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}
		}
	} else {
		if (is_all_same) {
			for (size_t i = 0; i < lhs_occupation_set.bit_size(); ++i) {
				if (!lhs_occupation_set.get_bit(i)) {
					is_all_same = false;
					break;
				}
			}
			for (size_t i = 0; i < rhs_occupation_set.bit_size(); ++i) {
				if (!rhs_occupation_set.get_bit(i)) {
					is_all_same = false;
					break;
				}
			}
		}
	}
	is_same_out = is_all_same;

	if (!diff_caches.insert({ GreenNodePin(lhs), GreenNodePin(rhs) }, +is_same_out))
		co_return GreenNodeOperationResult::OutOfMemory;

	co_return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeOperationResult ast::green_tree_diff(
	peff::Alloc *allocator,
	const GreenNodePin &lhs,
	const GreenNodePin &rhs,
	GreenNodeDiffSet &diff_set_out) {
	GreenNodeDiffCoroutineScheduler sched(allocator);
	GreenNodeDiffCache diff_cache(allocator);

	size_t path[1] = { 0 };
	bool same;
	auto result = _do_simple_green_tree_diff(allocator, sched, lhs, rhs, path, &diff_set_out, diff_cache, same).resume(&sched);

	if (result != GreenNodeOperationResult::Success)
		return result;

	return GreenNodeOperationResult::Success;
}
