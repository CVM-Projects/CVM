#pragma once
#include "basic.h"

namespace CVM {
	class ParseInfo;

	using ErrorCode = int;

	struct ParseUnit {
		explicit ParseUnit(ParseInfo &parseinfo, const PriLib::CharPtrView &raw)
				: parseinfo(parseinfo), raw(raw), currview(raw) {}

		ParseUnit(const ParseUnit &parseunit) = default;

		ParseUnit& operator=(const ParseUnit &parseunit) {
			new (this) ParseUnit(parseunit);
			return *this;
		}

		ParseInfo &parseinfo;
		const PriLib::CharPtrView &raw;
		PriLib::CharPtrView currview;
		ErrorCode errorcode = 0;
	};
}
