#pragma once

namespace CVM::InstStruct
{
	enum ElementType {
#define InstPart(key) ET_##key,
#include "instpart.def"
	};
}
