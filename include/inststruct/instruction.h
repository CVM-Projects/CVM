#pragma once
#include "instcode.h"

namespace CVM
{
	namespace InstStruct
	{
		struct Instruction
		{
			explicit Instruction(InstCode ic, uint8_t id = 0)
				: instcode(ic), _subid(id) {}

			InstCode instcode;
			uint8_t _subid;
		};
	}
}
