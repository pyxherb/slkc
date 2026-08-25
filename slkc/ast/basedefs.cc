#include "utils.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API DumpResult slkc::ast::dump_token_range(wandjson::ObjectValue *target_object, AstNodeDumpContext &dump_context, const TokenRange &token_range) {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint32_t>(token_range.source_node)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("src", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint32_t>(token_range.begin)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("begin", v.release()))
		return DumpResult::OutOfMemory;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint32_t>(token_range.end)))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("end", v.release()))
		return DumpResult::OutOfMemory;

	return DumpResult::Ok;
}

SLKC_API wandjson::StringValue *slkc::ast::dump_string(AstNodeDumpContext &dump_context, std::string_view sv) noexcept {
	return wandjson::StringValue::alloc(dump_context.get_allocator(), sv);
}
