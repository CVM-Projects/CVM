#pragma once
#include "hashstringpool.h"
#include "typeinfo.h"
#include "datapool.h"
#include "filenamemap.h"

namespace CVM
{
	namespace InstStruct
	{
	    enum DataRegisterMode {
	        drm_multiply = 0,
	        drm_dynamic,
	        drm_static,
	    };

		struct GlobalInfo {
			GlobalInfo() : typeInfoMap(hashStringPool) {}

			HashID entry;
			HashStringPool hashStringPool;
			DataRegisterMode dataRegisterMode = drm_multiply;
			TypeInfoMap typeInfoMap;
			NewLiteralDataPool literalDataPool;
			FileNameMap fileNameMap;
		};

		struct FileContext {
			GlobalInfo *globalInfo;
			FileID fileID;
			FileLiteralDataPoolMap *fileLiteralDataPoolMap;
		};
	}
}
