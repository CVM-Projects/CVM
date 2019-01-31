#pragma once
#include "basic.h"

namespace CVM {
    class ParseInfo;

    using ErrorCode = int;

    struct ParseUnit {
        explicit ParseUnit(ParseInfo &parseinfo, const PriLib::StringView &raw)
                : parseinfo(parseinfo), raw(raw), currview(raw) {}

        explicit ParseUnit(const ParseUnit &parseunit)
                : parseinfo(parseunit.parseinfo), raw(parseunit.raw), currview(parseunit.currview), errorcode(parseunit.errorcode) {}

        ParseUnit& operator=(const ParseUnit &parseunit) {
            new (this) ParseUnit(parseunit);
            return *this;
        }

        ParseInfo &parseinfo;
        const PriLib::StringView &raw;
        PriLib::StringView currview;
        ErrorCode errorcode = 0;
    };
}
