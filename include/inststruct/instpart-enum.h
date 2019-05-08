#pragma once

namespace CVM::InstStruct
{
	enum ElementType {
	    ET_Unknown,
#define InstPart(key) ET_##key,
#include "instpart.def"
	};
}
