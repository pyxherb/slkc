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

SLKC_API std::span<size_t> RedNodeChildIndices::get_classified_indices(TokenKind kind) {
	return _children_index.at(kind);
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
		if (children[j])
			continue;
		GreenNodeOperationResult result = build_child(allocator, j);
		if (result != GreenNodeOperationResult::Success)
			return result;
	}

	return GreenNodeOperationResult::Success;
}

SLAKE_API GreenNodeOperationResult RedNode::get_child_node(peff::Alloc *allocator, size_t index, RedNodePtr &red_node_out) noexcept {
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

	red_node_out = children[index];
	return GreenNodeOperationResult::Success;
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

SLAKE_API GreenNodeOperationResult GreenNodeDerecursedFnCoroutine::resume(GreenNodeDerecursedFnCoroutineScheduler *scheduler) {
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

SLKC_API GreenNodeDerecursedFnCoroutine::Awaitable::Awaitable(
	GreenNodeDerecursedFnCoroutine &co,
	GreenNodeDerecursedFnCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

SLKC_API bool GreenNodeDerecursedFnCoroutine::Awaitable::await_ready() {
	return false;
}

SLKC_API void GreenNodeDerecursedFnCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = GreenNodeOperationResult::OutOfMemory;
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = GreenNodeOperationResult::OutOfMemory;
		return;
	}
}

SLKC_API GreenNodeOperationResult GreenNodeDerecursedFnCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result != GreenNodeOperationResult::Success)
			return handle.promise().result;
		return GreenNodeOperationResult::Success;
	}
	return GreenNodeOperationResult::OutOfMemory;
}

SLKC_API GreenNodeDerecursedFnCoroutine::Awaitable GreenNodeDerecursedFnCoroutine::operator()(GreenNodeDerecursedFnCoroutineScheduler *scheduler) {
	return Awaitable(*this, scheduler, coro_handle);
}

SLKC_API GreenNodeDerecursedFnCoroutineScheduler::GreenNodeDerecursedFnCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

SLAKE_FORCEINLINE uint64_t hash_combine(uint64_t seed, uint64_t value) {
	seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
	return seed;
}

SLKC_API GreenNodeDerecursedFnCoroutine ast::_do_green_tree_hash(
	peff::Alloc *allocator,
	GreenNodeDerecursedFnCoroutineScheduler &scheduler,
	const GreenNodePin &lhs,
	GreenNodeHashCodeSet &hash_code_set,
	uint64_t &hash_out) {
	if (auto it = hash_code_set.find(lhs.get_index()); it != hash_code_set.end()) {
		hash_out = it.value();
		co_return GreenNodeOperationResult::Success;
	}

	uint64_t hash_code = lhs->node_kind;
	for (size_t i = 0; i < lhs->children.size(); ++i) {
		if (auto *token = std::get_if<TokenPtr>(&lhs->children[i])) {
			auto sv = token->get()->source_text.get_view();
			hash_code = hash_combine(hash_code, peff::city_hash64(sv.data(), sv.size()));
		} else if (auto *node = std::get_if<GreenNodePtr>(&lhs->children[i])) {
			if (auto it = hash_code_set.find(node->get_index()); it != hash_code_set.end()) {
				hash_code = hash_combine(hash_code, it.value());
			} else {
				uint64_t subhash = 0;
				auto pinned = node->pin();
				if (pinned.is_fail()) {
					co_return pin_fail_reason_to_green_node_operation_result(pinned.get_fail_reason());
				}

				if (auto result = co_await _do_green_tree_hash(allocator, scheduler, pinned, hash_code_set, subhash)(&scheduler); result != GreenNodeOperationResult::Success)
					co_return result;

				hash_code = hash_combine(hash_code, subhash);
			}
		}
	}

	hash_out = hash_code;

	if (!hash_code_set.insert(lhs.get_index(), +hash_code))
		co_return GreenNodeOperationResult::OutOfMemory;

	co_return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeOperationResult ast::green_node_hash(
	peff::Alloc *allocator,
	const GreenNodePin &lhs,
	GreenNodeHashCodeSet &hash_code_set_out,
	uint64_t &hash_out) {
	GreenNodeDerecursedFnCoroutineScheduler sched(allocator);

	size_t path[1] = { 0 };
	bool same;
	auto result = _do_green_tree_hash(allocator, sched, lhs, hash_code_set_out, hash_out).resume(&sched);

	if (result != GreenNodeOperationResult::Success)
		return result;

	return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeDerecursedFnCoroutine ast::_do_simple_green_tree_diff(
	peff::Alloc *allocator,
	GreenNodeDerecursedFnCoroutineScheduler &scheduler,
	const GreenNodePin &lhs,
	const GreenNodePin &rhs,
	std::span<size_t> lhs_path_base,
	const GreenNodeHashCodeSet &lhs_hash_code_set,
	const GreenNodeHashCodeSet &rhs_hash_code_set,
	GreenNodeDiffSet *diff_set_out,
	bool &is_same_out) noexcept {
	// As we tested, caching comparison result of every combination we meet
	// consumes too many memory and there was no obvious performance
	// improvements.assert(lhs);
	assert(rhs);
	assert(lhs->node_kind == rhs->node_kind);

	{
		auto lhc = lhs_hash_code_set.find(lhs.get_index());
		auto rhc = rhs_hash_code_set.find(rhs.get_index());
		assert(lhc != lhs_hash_code_set.end());
		assert(rhc != rhs_hash_code_set.end());
		if (lhc.value() == rhc.value()) {
			is_same_out = true;
			co_return GreenNodeOperationResult::Success;
		}
		if (!diff_set_out) {
			is_same_out = false;
			co_return GreenNodeOperationResult::Success;
		}
	}

	// Pre-extract hash code of all children.
	peff::DynArray<uint64_t> lhs_hash_cache(allocator);
	peff::DynArray<uint64_t> rhs_hash_cache(allocator);
	if (!lhs_hash_cache.resize(lhs->children.size()) ||
		!rhs_hash_cache.resize(rhs->children.size()))
		co_return GreenNodeOperationResult::OutOfMemory;

	for (size_t i = 0; i < lhs->children.size(); ++i) {
		if (auto *node = std::get_if<GreenNodePtr>(&lhs->children[i])) {
			auto it = lhs_hash_code_set.find(node->get_index());
			lhs_hash_cache[i] = (it != lhs_hash_code_set.end()) ? it.value() : 0;
		} else {
			// Can't use hash for tokens.
			lhs_hash_cache[i] = 0;
		}
	}
	for (size_t j = 0; j < rhs->children.size(); ++j) {
		if (auto *node = std::get_if<GreenNodePtr>(&rhs->children[j])) {
			auto it = rhs_hash_code_set.find(node->get_index());
			rhs_hash_cache[j] = (it != rhs_hash_code_set.end()) ? it.value() : 0;
		} else {
			rhs_hash_cache[j] = 0;
		}
	}

	peff::BitArray lhs_occupation_set(allocator);
	peff::BitArray rhs_occupation_set(allocator);
	peff::BTreeMap<size_t, size_t> lhs_to_rhs_move_map(allocator);

	if (!lhs_occupation_set.resize(lhs->children.size()))
		co_return GreenNodeOperationResult::OutOfMemory;
	if (!rhs_occupation_set.resize(rhs->children.size()))
		co_return GreenNodeOperationResult::OutOfMemory;

	peff::DynArray<size_t> extended_path(allocator);
	if (!extended_path.build(lhs_path_base))
		co_return GreenNodeOperationResult::OutOfMemory;
	if (!extended_path.push_back(SIZE_MAX))
		co_return GreenNodeOperationResult::OutOfMemory;

	auto compute_lis = [allocator](const peff::DynArray<size_t> &values,
						   peff::DynArray<size_t> &out_indices) -> bool {
		out_indices.clear();
		size_t n = values.size();
		if (n == 0)
			return true;

		peff::DynArray<size_t> tails(allocator);
		peff::DynArray<size_t> tail_indices(allocator);
		peff::DynArray<size_t> prev(allocator);
		if (!tails.resize_uninit(n) || !tail_indices.resize_uninit(n) || !prev.resize_uninit(n))
			return false;

		size_t len = 0;
		for (size_t i = 0; i < n; ++i) {
			size_t val = values[i];
			size_t lo = 0, hi = len;
			while (lo < hi) {
				size_t mid = lo + (hi - lo) / 2;
				if (tails[mid] < val)
					lo = mid + 1;
				else
					hi = mid;
			}
			if (lo == len)
				++len;
			tails[lo] = val;
			tail_indices[lo] = i;
			prev[i] = (lo > 0) ? tail_indices[lo - 1] : SIZE_MAX;
		}

		if (!out_indices.resize_uninit(len))
			return false;
		size_t idx = tail_indices[len - 1];
		for (size_t k = len; k > 0; --k) {
			out_indices[k - 1] = idx;
			idx = prev[idx];
		}
		return true;
	};

	// Exact matching via LCS.
	{
		size_t n = lhs->children.size();
		size_t m = rhs->children.size();

		peff::DynArray<size_t> dp_prev(allocator);
		peff::DynArray<size_t> dp_curr(allocator);
		peff::DynArray<peff::DynArray<uint8_t>> choice(allocator);

		if (!dp_prev.resize_uninit(m + 1) || !dp_curr.resize_uninit(m + 1))
			co_return GreenNodeOperationResult::OutOfMemory;
		if (!choice.resize_construct(n + 1, allocator))
			co_return GreenNodeOperationResult::OutOfMemory;
		for (size_t i = 0; i <= n; ++i) {
			if (!choice[i].resize_uninit(m + 1))
				co_return GreenNodeOperationResult::OutOfMemory;
		}

		for (size_t j = 0; j <= m; ++j) {
			dp_prev[j] = 0;
			choice[0][j] = 1;
		}

		for (size_t i = 1; i <= n; ++i) {
			dp_curr[0] = 0;
			choice[i][0] = 0;

			auto &lhs_child = lhs->children[i - 1];
			bool lhs_is_token = std::holds_alternative<TokenPtr>(lhs_child);
			TokenPtr lhs_token = lhs_is_token ? *std::get_if<TokenPtr>(&lhs_child) : TokenPtr{};
			GreenNodePtr lhs_green = !lhs_is_token ? *std::get_if<GreenNodePtr>(&lhs_child) : GreenNodePtr{};

			for (size_t j = 1; j <= m; ++j) {
				// ?
				if (dp_prev[j] >= dp_curr[j - 1]) {
					dp_curr[j] = dp_prev[j];
					choice[i][j] = 0;
				} else {
					dp_curr[j] = dp_curr[j - 1];
					choice[i][j] = 1;
				}

				size_t li = i - 1;
				size_t rj = j - 1;
				if (lhs_occupation_set.get_bit(li) || rhs_occupation_set.get_bit(rj))
					continue;

				auto &rhs_child = rhs->children[rj];
				bool rhs_is_token = std::holds_alternative<TokenPtr>(rhs_child);
				if (lhs_is_token != rhs_is_token)
					continue;

				if (lhs_is_token) {
					TokenPtr rhs_token = *std::get_if<TokenPtr>(&rhs_child);
					if (lhs_token->token_id != rhs_token->token_id)
						continue;
					if (lhs_token->source_text.get_view() == rhs_token->source_text.get_view()) {
						if (dp_prev[j - 1] + 1 > dp_curr[j]) {
							dp_curr[j] = dp_prev[j - 1] + 1;
							choice[i][j] = 2;
						}
					}
				} else {
					const GreenNodePtr &rhs_green = *std::get_if<GreenNodePtr>(&rhs_child);

					if (lhs_green.get_index() == rhs_green.get_index()) {
						if (dp_prev[j - 1] + 1 > dp_curr[j]) {
							dp_curr[j] = dp_prev[j - 1] + 1;
							choice[i][j] = 2;
						}
						continue;
					}

					// Use hash instead of recursing.
					uint64_t lh = lhs_hash_cache[li];
					uint64_t rh = rhs_hash_cache[rj];
					if (lh != 0 && rh != 0 && lh == rh) {
						if (dp_prev[j - 1] + 1 > dp_curr[j]) {
							dp_curr[j] = dp_prev[j - 1] + 1;
							choice[i][j] = 2;
						}
						continue;
					}
				}
			}
			std::swap(dp_prev, dp_curr);
		}

		// Extract exact matches.
		size_t i = n, j = m;
		while (i > 0 && j > 0) {
			if (choice[i][j] == 2) {
				size_t li = i - 1;
				size_t rj = j - 1;
				assert(!lhs_occupation_set.get_bit(li));
				assert(!rhs_occupation_set.get_bit(rj));
				assert(!lhs_to_rhs_move_map.contains(li));

				if (!lhs_to_rhs_move_map.insert(+li, +rj))
					co_return GreenNodeOperationResult::OutOfMemory;
				lhs_occupation_set.set_bit(li);
				rhs_occupation_set.set_bit(rj);
				--i;
				--j;
			} else if (choice[i][j] == 0) {
				--i;
			} else {
				--j;
			}
		}
	}

	// Kind-only match via LCS
	{
		size_t n = lhs->children.size();
		size_t m = rhs->children.size();

		peff::DynArray<size_t> dp_prev(allocator);
		peff::DynArray<size_t> dp_curr(allocator);
		peff::DynArray<peff::DynArray<uint8_t>> choice(allocator);

		if (!dp_prev.resize_uninit(m + 1) || !dp_curr.resize_uninit(m + 1))
			co_return GreenNodeOperationResult::OutOfMemory;
		if (!choice.resize_construct(n + 1, allocator))
			co_return GreenNodeOperationResult::OutOfMemory;
		for (size_t i = 0; i <= n; ++i) {
			if (!choice[i].resize_uninit(m + 1))
				co_return GreenNodeOperationResult::OutOfMemory;
		}

		for (size_t j = 0; j <= m; ++j) {
			dp_prev[j] = 0;
			choice[0][j] = 1;
		}

		for (size_t i = 1; i <= n; ++i) {
			dp_curr[0] = 0;
			choice[i][0] = 0;

			auto &lhs_child = lhs->children[i - 1];
			bool lhs_is_token = std::holds_alternative<TokenPtr>(lhs_child);
			TokenPtr lhs_token = lhs_is_token ? *std::get_if<TokenPtr>(&lhs_child) : TokenPtr{};
			GreenNodePtr lhs_green = !lhs_is_token ? *std::get_if<GreenNodePtr>(&lhs_child) : GreenNodePtr{};

			for (size_t j = 1; j <= m; ++j) {
				if (dp_prev[j] >= dp_curr[j - 1]) {
					dp_curr[j] = dp_prev[j];
					choice[i][j] = 0;
				} else {
					dp_curr[j] = dp_curr[j - 1];
					choice[i][j] = 1;
				}

				size_t li = i - 1;
				size_t rj = j - 1;
				if (lhs_occupation_set.get_bit(li) || rhs_occupation_set.get_bit(rj))
					continue;

				auto &rhs_child = rhs->children[rj];
				bool rhs_is_token = std::holds_alternative<TokenPtr>(rhs_child);
				if (lhs_is_token != rhs_is_token)
					continue;
				if (lhs_is_token) {
					TokenPtr rhs_token = *std::get_if<TokenPtr>(&rhs_child);
					if (lhs_token->token_id != rhs_token->token_id)
						continue;
				} else {
					const GreenNodePtr &rhs_green = *std::get_if<GreenNodePtr>(&rhs_child);

					if (lhs_green.get_index() == rhs_green.get_index()) {
						if (dp_prev[j - 1] + 1 > dp_curr[j]) {
							dp_curr[j] = dp_prev[j - 1] + 1;
							choice[i][j] = 2;
						}
						continue;
					}
					auto pinned_lhs = lhs_green.pin();
					if (pinned_lhs.is_fail())
						co_return pin_fail_reason_to_green_node_operation_result(pinned_lhs.get_fail_reason());
					auto pinned_rhs = rhs_green.pin();
					if (pinned_rhs.is_fail())
						co_return pin_fail_reason_to_green_node_operation_result(pinned_rhs.get_fail_reason());
					if (pinned_lhs->node_kind == pinned_rhs->node_kind) {
						if (dp_prev[j - 1] + 1 > dp_curr[j]) {
							dp_curr[j] = dp_prev[j - 1] + 1;
							choice[i][j] = 2;
						}
					}
				}
			}
			std::swap(dp_prev, dp_curr);
		}

		// Go back to extract the kind matches.
		size_t i = n, j = m;
		while (i > 0 && j > 0) {
			if (choice[i][j] == 2) {
				size_t li = i - 1;
				size_t rj = j - 1;
				assert(!lhs_occupation_set.get_bit(li));
				assert(!rhs_occupation_set.get_bit(rj));
				assert(!lhs_to_rhs_move_map.contains(li));

				if (!lhs_to_rhs_move_map.insert(+li, +rj))
					co_return GreenNodeOperationResult::OutOfMemory;
				lhs_occupation_set.set_bit(li);
				rhs_occupation_set.set_bit(rj);
				--i;
				--j;
			} else if (choice[i][j] == 0) {
				--i;
			} else {
				--j;
			}
		}
	}

	// Generate content differences for kind-matched (non-exact) nodes
	for (size_t i = 0; i < lhs->children.size(); ++i) {
		if (!lhs_occupation_set.get_bit(i))
			continue;
		auto it = lhs_to_rhs_move_map.find(i);
		assert(it != lhs_to_rhs_move_map.end());
		size_t j = it.value();

		if (auto lit = std::get_if<GreenNodePtr>(&lhs->children[i]); lit) {
			auto rit = std::get_if<GreenNodePtr>(&rhs->children[j]);
			assert(rit);

			if (lit->get_index() == rit->get_index())
				continue;

			auto pinned_lhs = lit->pin();
			if (pinned_lhs.is_fail())
				co_return pin_fail_reason_to_green_node_operation_result(pinned_lhs.get_fail_reason());
			auto pinned_rhs = rit->pin();
			if (pinned_rhs.is_fail())
				co_return pin_fail_reason_to_green_node_operation_result(pinned_rhs.get_fail_reason());

			extended_path.back() = i;
			bool same = true;
			if (auto result = co_await _do_simple_green_tree_diff(
					allocator, scheduler, pinned_lhs, pinned_rhs,
					extended_path, lhs_hash_code_set, rhs_hash_code_set, diff_set_out, same)(&scheduler);
				result != GreenNodeOperationResult::Success)
				co_return result;
		} else {
			TokenPtr lhs_token = *std::get_if<TokenPtr>(&lhs->children[i]);
			auto rit = std::get_if<TokenPtr>(&rhs->children[j]);
			assert(rit);
			TokenPtr rhs_token = *rit;

			if (lhs_token->token_id == rhs_token->token_id &&
				lhs_token->source_text.get_view() != rhs_token->source_text.get_view()) {
				// Token content changed
				extended_path.back() = i;
				GreenNodeDiff diff(allocator);
				diff.kind = GreenNodeDiffKind::ReplacedLhsNode;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}
		}
	}

	// Unified movement detection using LIS on all matched pairs
	peff::DynArray<size_t> matched_lhs(allocator);
	peff::DynArray<size_t> matched_rhs(allocator);
	peff::DynArray<size_t> lis_indices(allocator);

	for (size_t i = 0; i < lhs->children.size(); ++i) {
		if (auto it = lhs_to_rhs_move_map.find(i); it != lhs_to_rhs_move_map.end()) {
			if (!matched_lhs.push_back(+i))
				co_return GreenNodeOperationResult::OutOfMemory;
			if (!matched_rhs.push_back(+it.value()))
				co_return GreenNodeOperationResult::OutOfMemory;
		}
	}

	if (!compute_lis(matched_rhs, lis_indices))
		co_return GreenNodeOperationResult::OutOfMemory;

	peff::BitArray kept_in_order(allocator);
	if (!kept_in_order.resize_uninit(matched_rhs.size()))
		co_return GreenNodeOperationResult::OutOfMemory;
	for (size_t idx : lis_indices)
		kept_in_order.set_bit(idx);

	if (diff_set_out) {
		for (size_t idx = 0; idx < matched_rhs.size(); ++idx) {
			if (kept_in_order.get_bit(idx))
				continue;
			size_t lhs_idx = matched_lhs[idx];
			size_t rhs_idx = matched_rhs[idx];
			if (lhs_idx == rhs_idx)
				continue;

			extended_path.back() = lhs_idx;
			GreenNodeDiff diff(allocator);
			diff.kind = GreenNodeDiffKind::Moved;
			diff.exdata.moved.moved_to_index = rhs_idx;
			if (!diff.path.build(extended_path))
				co_return GreenNodeOperationResult::OutOfMemory;
			if (!diff_set_out->insert(std::move(diff)))
				co_return GreenNodeOperationResult::OutOfMemory;
		}
	}

	// Generate removed and inserted diffs for unmatched nodes
	bool is_all_same = true;

	if (lhs_occupation_set.size() != rhs_occupation_set.size()) {
		is_all_same = false;
	} else {
		for (size_t i = 0; i < lhs_occupation_set.size(); ++i) {
			if (!lhs_occupation_set.get_bit(i)) {
				is_all_same = false;
				break;
			}
		}
		if (is_all_same) {
			for (size_t i = 0; i < rhs_occupation_set.size(); ++i) {
				if (!rhs_occupation_set.get_bit(i)) {
					is_all_same = false;
					break;
				}
			}
		}
	}

	if (is_all_same) {
		for (size_t i = 0; i < matched_lhs.size(); ++i) {
			if (matched_lhs[i] != matched_rhs[i]) {
				is_all_same = false;
				break;
			}
		}
	}

	if (diff_set_out) {
		for (size_t i = 0; i < lhs_occupation_set.size(); ++i) {
			if (!lhs_occupation_set.get_bit(i)) {
				extended_path.back() = i;
				GreenNodeDiff diff(allocator);
				diff.kind = GreenNodeDiffKind::RemovedFromLhs;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}
		}
		for (size_t i = 0; i < rhs_occupation_set.size(); ++i) {
			if (!rhs_occupation_set.get_bit(i)) {
				extended_path.back() = i;
				GreenNodeDiff diff(allocator);
				diff.kind = GreenNodeDiffKind::InsertedIntoRhs;
				diff.is_dest_path = true;
				if (!diff.path.build(extended_path))
					co_return GreenNodeOperationResult::OutOfMemory;
				if (!diff_set_out->insert(std::move(diff)))
					co_return GreenNodeOperationResult::OutOfMemory;
			}
		}
	}

	is_same_out = is_all_same;

	co_return GreenNodeOperationResult::Success;
}

SLKC_API GreenNodeOperationResult ast::green_tree_diff(
	peff::Alloc *allocator,
	const GreenNodePin &lhs,
	const GreenNodePin &rhs,
	const GreenNodeHashCodeSet &lhs_hash_code_set,
	const GreenNodeHashCodeSet &rhs_hash_code_set,
	GreenNodeDiffSet &diff_set_out) {
	GreenNodeDerecursedFnCoroutineScheduler sched(allocator);

	size_t path[1] = { 0 };
	bool same;
	auto result = _do_simple_green_tree_diff(allocator, sched, lhs, rhs, path, lhs_hash_code_set, rhs_hash_code_set, &diff_set_out, same).resume(&sched);

	if (result != GreenNodeOperationResult::Success)
		return result;

	return GreenNodeOperationResult::Success;
}
