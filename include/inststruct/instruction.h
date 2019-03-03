#pragma once
#include "instcode.h"

namespace CVM
{
	namespace InstStruct
	{
		struct Element;
		
		// TODO
		struct Instruction {
			explicit Instruction(InstCode ic, std::vector<Element*> data)
				: instcode(ic), data(std::move(data)) {}

			virtual ~Instruction() {}

			InstCode instcode;
			std::vector<Element*> data;
		};
	}
}
