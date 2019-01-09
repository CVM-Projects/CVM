#pragma once
#include <memory>
#include "../prilib/include/file.h"
#include "../prilib/include/storeptr.h"
#include "typeinfo.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/identkeytable.h"
#include "datapool.h"
#include "inststruct/info.h"

namespace CVM
{
	class ParseInfo;

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim);
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);

	HashID getEntry(ParseInfo &parseinfo);
	const std::string& getFromHashStringPool(ParseInfo &parseinfo, HashID id);
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo &parseinfo);
	InstStruct::IdentKeyTable& getFunctionTable(ParseInfo &parseinfo);
	bool haveError(const ParseInfo &parseinfo);
}
