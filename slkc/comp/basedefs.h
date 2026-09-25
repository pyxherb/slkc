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

		class TypeDef;

		SLKC_API std::strong_ordering _compare_type_defs(const TypeDef *lhs, const TypeDef *rhs);

		struct TypeDefComparator {
			SLAKE_FORCEINLINE std::strong_ordering operator()(TypeDef *lhs, const TypeDef *rhs) const noexcept {
				return _compare_type_defs(lhs, rhs);
			}
		};
	}
}

#endif
