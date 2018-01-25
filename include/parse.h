#pragma once
#include <memory>
#include "../prilib/include/file.h"
#include "../prilib/include/storeptr.h"
#include "typeinfo.h"
#include "inststruct/function.h"

namespace CVM
{
	class ParseInfo;

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim);
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);

	using FunctionSet = std::map<std::string, InstStruct::Function*>;
	FunctionSet createFunctionSet(ParseInfo &parseinfo);
}
