#include "type.h"

using namespace slkc;
using namespace slkc::ast;

SLKC_API DumpResult slkc::ast::dump_typename(wandjson::ObjectValue *target_object, DumpContext &dump_context, const TypeName &tn, bool deep_dump) {
	std::unique_ptr<wandjson::Value, wandjson::ValueDeleter> v;

	if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint8_t>(tn.get_typename_kind())))))
		return DumpResult::OutOfMemory;
	if (!target_object->insert("kind", v.release()))
		return DumpResult::OutOfMemory;

	if (auto def = tn.get_def(); def) {
		if (deep_dump) {
			if (!(v = decltype(v)(wandjson::ObjectValue::alloc(dump_context.get_allocator()))))
				return DumpResult::OutOfMemory;
			SLKC_RETURN_IF_DUMP_FAILED(dump_context.push_task(static_cast<wandjson::ObjectValue *>(v.get()), def.get_index(), deep_dump));
			if (!target_object->insert("def", v.release()))
				return DumpResult::OutOfMemory;
		} else {
			if (!(v = decltype(v)(wandjson::NumberValue::alloc_int(dump_context.get_allocator(), static_cast<uint32_t>(def)))))
				return DumpResult::OutOfMemory;
			if (!target_object->insert("def", v.release()))
				return DumpResult::OutOfMemory;
		}
	}

	return DumpResult::Ok;
}
