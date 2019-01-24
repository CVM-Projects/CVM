#pragma once
#include "hashstringpool.h"

namespace CVM
{
	namespace InstStruct
	{
	    enum DataRegisterMode {
	        drm_multiply = 0,
	        drm_dynamic,
	        drm_static,
	    };
		struct GlobalInfo
		{
			HashID entry;
			HashStringPool hashStringPool;
			DataRegisterMode dataRegisterMode = drm_multiply;
		};
	}
}
