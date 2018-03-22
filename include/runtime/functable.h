#pragma once
#include "function.h"
#include "../../prilib/include/bijectionmap.h"

namespace CVM
{
	namespace Runtime
	{
		using FuncTable = std::map<size_t, Function*>;
	}
}
