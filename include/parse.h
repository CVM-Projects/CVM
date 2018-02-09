#pragma once
#include <memory>
#include "../prilib/include/file.h"
#include "../prilib/include/storeptr.h"
#include "typeinfo.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "datapool.h"

namespace CVM
{
	class ParseInfo;

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim);
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);

	using FunctionSet = std::map<std::string, InstStruct::Function*>;
	FunctionSet createFunctionSet(ParseInfo &parseinfo);

	std::string getEntry(ParseInfo &parseinfo);
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo &parseinfo);
	bool haveError(const ParseInfo &parseinfo);
}
