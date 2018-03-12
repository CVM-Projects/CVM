#pragma once
#include "function.h"
#include "../../prilib/include/bijectionmap.h"

namespace CVM
{
	namespace Runtime
	{
		using FuncTable = PriLib::BijectionMap<std::string, Function*>;
	}
}
