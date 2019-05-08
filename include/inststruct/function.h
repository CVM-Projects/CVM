#pragma once
#include <vector>
#include <utility>
#include "funcinfo.h"
#include "config.h"
#include "instruction.h"

namespace CVM::InstStruct
{
	using InstList = std::vector<Instruction*>; // TODO
	using LabelKeyTable = std::map<HashID, Config::LineCountType>;

	struct Function
	{
		explicit Function() = default;
		explicit Function(const Function &info) = default;

		explicit Function(Function &&func)
			: info(std::move(func.info)), instdata(std::move(func.instdata)), labelkeytable(std::move(func.labelkeytable)) {}

		~Function() {
			// TODO
			for (auto &p : instdata)
				if (p)
					delete p;
		}

		FunctionInfo info;
		InstList instdata;
		LabelKeyTable labelkeytable;
	};
}
