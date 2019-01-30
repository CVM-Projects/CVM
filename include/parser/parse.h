#pragma once
#include <memory>
#include "basic.h"
#include "typeinfo.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/identkeytable.h"
#include "datapool.h"
#include "inststruct/info.h"
#include "parseunit.h"

namespace CVM
{
	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim);
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);

	HashID getEntry(ParseInfo &parseinfo);
	const std::string& getFromHashStringPool(ParseInfo &parseinfo, HashID id);
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo &parseinfo);
	InstStruct::IdentKeyTable& getFunctionTable(ParseInfo &parseinfo);
	bool haveError(const ParseInfo &parseinfo);
	InstStruct::GlobalInfo& getGlobalInfo(ParseInfo &parseinfo);
	bool isEndChar(ParseInfo &parseinfo, char c);
	bool isIdentifierChar(ParseInfo &parseinfo, char c);
	bool hasIdentifierPrefix(ParseInfo &parseinfo, const char *str);
	bool matchPrefix(ParseUnit &parseunit, const PriLib::StringView &substr);
	bool matchPrefix(ParseUnit &parseunit, char subchar);
}
