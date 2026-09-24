#ifndef _SLKC_AST_BASEDEFS_H_
#define _SLKC_AST_BASEDEFS_H_

#include <slkc/basedefs.h>
#include <wandjson/dump.h>
#include <cstdint>
#include <limits>

namespace slkc {
	namespace ast {
		using AstNodeIndex = uint64_t;
		using GreenNodeIndex = uint64_t;
		using TokenIndex = uint32_t;
		using TokenKind = uint32_t;
		using TextWidth = size_t;

		constexpr AstNodeIndex INVALID_AST_NODE_INDEX = (std::numeric_limits<AstNodeIndex>::max)();
		constexpr GreenNodeIndex INVALID_GREEN_NODE_INDEX = (std::numeric_limits<GreenNodeIndex>::max)();
		constexpr TokenIndex INVALID_TOKEN_INDEX = (std::numeric_limits<TokenIndex>::max)();

		struct TokenRange {
			AstNodeIndex source_node;
			TokenIndex begin, end;

			TokenRange() = default;
			SLAKE_FORCEINLINE TokenRange(AstNodeIndex source_node, TokenIndex begin, TokenIndex end)
				: source_node(source_node), begin(begin), end(end) {}
			SLAKE_FORCEINLINE TokenRange(AstNodeIndex source_node, TokenIndex index)
				: source_node(source_node), begin(index), end(index) {}
		};

		enum class DumpResult : uint8_t {
			Ok,
			OutOfMemory,
			PinningFailed
		};

		enum class DuplicationError : uint8_t {
			NoSlot,
			OutOfMemory,
			PinningFailed
		};

		struct AstNodeDumpContext;

		SLKC_API DumpResult dump_token_range(wandjson::ObjectValue *target_object, AstNodeDumpContext &dump_context, const TokenRange &token_range);
		SLKC_API wandjson::StringValue *dump_string(AstNodeDumpContext &dump_context, std::string_view sv) noexcept;
	}
}

#define SLKC_RETURN_IF_DUMP_FAILED(e)                           \
	do {                                                        \
		if (slkc::ast::DumpResult _ = (e); _ != slkc::ast::DumpResult::Ok) \
			return _;                                           \
	} while (0)

#endif
