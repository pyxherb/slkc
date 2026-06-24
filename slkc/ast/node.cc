#include "global.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API Node::Node(NodeType ast_node_type, Global *global)
	: _ast_node_type(ast_node_type),
	  _global(global) {
}

SLKC_API Node::Node(const Node &other, DuplicationContext &context)
	: _ast_node_type(other._ast_node_type),
	  _global(other._global),
	  _token_range(other._token_range) {
}

SLKC_API Node::~Node() {
}

SLKC_API DumpResult Node::do_dump(DumpContext &dump_context, wandjson::ObjectValue *value_out, bool deep_dump) const noexcept {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(get_global()->get_allocator(), static_cast<uint8_t>(get_ast_node_type())))))
		return DumpResult::OutOfMemory;
	if (!value_out->insert("node_type", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}
