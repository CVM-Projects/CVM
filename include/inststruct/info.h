#pragma once
#include "hashstringpool.h"
#include "typeinfo.h"
#include "datapool.h"
#include "parser/datapool-creator.h"
#include "filenamemap.h"
#include "identkeytable.h"
#include <memory>

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
			GlobalInfo() : typeInfoMap(hashStringPool), literalDataPoolCreator(new LiteralDataPoolCreator()) {}

			HashID entry;
			HashStringPool hashStringPool;
			DataRegisterMode dataRegisterMode = drm_multiply;
			TypeInfoMap typeInfoMap;
			std::unique_ptr<LiteralDataPoolCreator> literalDataPoolCreator;  // TODO: Remove from GlobalInfo
			LiteralDataPool literalDataPool;
			FileNameMap fileNameMap;
			IdentKeyTable funcTable;
		};

		struct FileContext {
			GlobalInfo *globalInfo;
			FileID fileID;
			LiteralDataPoolMap *fileLiteralDataPoolMap;
		};
	}
}
