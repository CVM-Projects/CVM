#pragma once
#include "instcode.h"

namespace CVM::InstStruct
{
	struct Element;

	struct Instruction final {
		explicit Instruction(InstCode ic, std::vector<Element> data)
			: instcode(ic), data(std::move(data)) {}

		InstCode instcode;
		std::vector<Element> data;
	};
}
