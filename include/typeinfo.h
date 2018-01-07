#pragma once
#include "explicittype.h"
#include <cstdint>
#include <map>

namespace CVM
{
	using TypeIndex = Common::ExplicitType<uint32_t, 0>;
	struct TypeIndexLess {
		bool operator()(const TypeIndex &t1, const TypeIndex &t2) const {
			return t1.data < t2.data;
		}
	};

	using MemorySize = Common::ExplicitType<uint32_t, 0>;
	using TypeName = Common::ExplicitType<const char*, nullptr>;

	struct TypeInfo
	{
		TypeIndex index;
		MemorySize size;
		TypeName name;
	};

	using TypeInfoMap = std::map<TypeIndex, TypeInfo, TypeIndexLess>;
}
