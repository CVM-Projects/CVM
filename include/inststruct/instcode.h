#pragma once
#include <cstdint>

namespace CVM
{
	namespace InstStruct
	{
		enum InstCode : uint8_t {
#define InstCode(code) i_##code,
#define InstCodeDebug(code) id_##code,
#include "instcode.def"
		};
	}
}
