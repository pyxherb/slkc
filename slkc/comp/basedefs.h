#ifndef _SLKC_COMP_BASEDEFS_H_
#define _SLKC_COMP_BASEDEFS_H_

#include <slkc/basedefs.h>
#include <wandjson/dump.h>
#include <cstdint>
#include <limits>

namespace slkc {
	namespace comp {
		using TypeDefIndex = uint64_t;

		constexpr TypeDefIndex INVALID_TYPE_DEF_INDEX = (std::numeric_limits<TypeDefIndex>::max)();
	}
}

#endif
