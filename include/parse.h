#pragma once
#include <memory>
#include "../prilib/include/file.h"
#include "../prilib/include/storeptr.h"
#include "typeinfo.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/identkeytable.h"
#include "datapool.h"

namespace CVM
{
	class ParseInfo;

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim);
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);

	std::string getEntry(ParseInfo &parseinfo);
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo &parseinfo);
	InstStruct::IdentKeyTable& getFunctionTable(ParseInfo &parseinfo);
	bool haveError(const ParseInfo &parseinfo);
}
