#pragma once
#include "funcinfo.h"
#include "config.h"
#include "instruction.h"

namespace CVM
{
	namespace InstStruct
	{
		using InstList = std::vector<Instruction*>; // TODO

		struct Function
		{
			explicit Function() = default;
			explicit Function(const Function &info) = default;

			explicit Function(Function &&func)
				: info(std::move(func.info)), instdata(func.instdata) {}

			~Function() {
				for (auto &p : instdata)
					if (p)
						delete p;
			}

			FunctionInfo info;
			InstList instdata;
		};
	}
}
