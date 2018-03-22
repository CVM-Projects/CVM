#pragma once
#include "funcinfo.h"
#include "config.h"
#include "instruction.h"

namespace CVM
{
	namespace InstStruct
	{
		using InstList = std::vector<Instruction*>; // TODO

		struct Function : public FunctionInfo
		{
			explicit Function() = default;
			explicit Function(const Function &info) = default;

			explicit Function(Function &&info)
				: instdata(info.instdata), FunctionInfo(std::move(info)) {}

			explicit Function(InstList &&il, Config::RegisterIndexType &&dysize, TypeList &&stvarb_typelist, ArgList &&arglist) :
				instdata(std::move(il)),
				FunctionInfo(std::move(dysize), std::move(stvarb_typelist), std::move(arglist)) {}

			~Function() {
				for (auto &p : instdata)
					if (p)
						delete p;
			}

			InstList instdata;
		};
	}
}
