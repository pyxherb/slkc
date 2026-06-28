#ifndef _SLKC_AST_BASEDEFS_H_
#define _SLKC_AST_BASEDEFS_H_

#include <slkc/basedefs.h>
#include <wandjson/dump.h>
#include <cstdint>
#include <limits>

namespace slkc {
	namespace ast {
		using NodeIndex = uint32_t;
		using TokenIndex = uint32_t;

		constexpr NodeIndex INVALID_NODE_INDEX = std::numeric_limits<NodeIndex>::max();

		struct TokenRange {
			NodeIndex source_node;
			TokenIndex begin, end;
		};

		enum class DumpResult {
			Ok,
			OutOfMemory,
			PinningFailed
		};

		struct DumpContext;

		SLKC_API DumpResult dump_token_range(wandjson::ObjectValue *target_object, DumpContext &dump_context, const TokenRange &token_range);
		SLKC_API wandjson::StringValue *dump_string(DumpContext &dump_context, std::string_view sv) noexcept;
	}
}

#define SLKC_RETURN_IF_DUMP_FAILED(e)                           \
	do {                                                        \
		if (slkc::ast::DumpResult _ = (e); _ != DumpResult::Ok) \
			return _;                                           \
	} while (0)

#endif
