#pragma once
#include <memory>
#include "file.h"
#include "storeptr.h"

namespace CVM
{
	class ParseInfo;

	PriLib::StorePtr<ParseInfo> createParseInfo();
	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file);
}
