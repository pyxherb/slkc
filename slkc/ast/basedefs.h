#ifndef _SLKC_AST_BASEDEFS_H_
#define _SLKC_AST_BASEDEFS_H_

#include <slkc/basedefs.h>
#include <cstdint>
#include <limits>

namespace slkc {
	namespace ast {
		using NodeIndex = uint32_t;
		using TokenIndex = size_t;

		constexpr NodeIndex INVALID_NODE_INDEX = std::numeric_limits<NodeIndex>::max();

		struct TokenRange {
			NodeIndex source_node;
			TokenIndex begin, end;
		};
	}
}

#endif
