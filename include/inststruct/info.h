#pragma once
#include "hashstringpool.h"

namespace CVM
{
	namespace InstStruct
	{
		struct GlobalInfo
		{
			HashID entry;
			HashStringPool hashStringPool;
		};
	}
}
