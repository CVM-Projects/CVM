#include "basic.h"
#include "parser/parse-inststruct.h"
#include "parser/parse.h"

namespace CVM
{
    namespace Parse
    {
        using namespace InstStruct;

        static bool ParseRegisterScopePrefix(ParseUnit &parseunit, RegisterScopeType &scopetype) {
            // Parse '%t', '%g', '%'
            if (!matchPrefix(parseunit, "%"))
                return false;
            switch (parseunit.currview[0]) {
            case 't': scopetype = rst_thread; parseunit.currview += 1; break;
            case 'g': scopetype = rst_global; parseunit.currview += 1; break;
            default: scopetype = rst_local; break;
            }
            return true;
        }
        static bool ParseStackRegisterPrefix(ParseUnit &parseunit, RegisterScopeType &scopetype) {
            // Parse '%tsp', '%gsp', '%sp'
            return ParseRegisterScopePrefix(parseunit, scopetype) && matchPrefix(parseunit, "sp");
        }
        template <typename NumType>
        static bool ParseNumber(ParseUnit &parseunit, NumType &result, bool allow_negative = false) {
            const char *ptr = parseunit.currview.get();
            const char *endptr = parseunit.currview.get();
            if (allow_negative && (*ptr == '-') || (*ptr == '+'))
                ++endptr;
            while (isdigit(*endptr))
                ++endptr;
            if (ptr == endptr)
                return false;
            if (!PriLib::Convert::to_integer<NumType>(PriLib::StringViewRange(ptr, endptr).toString() /* TODO */, result))
                return false;
            parseunit.currview = PriLib::StringView(endptr);
            return true;
        }
        static bool ParseStackOffset(ParseUnit &parseunit, StackOffset &result) {
            return matchPrefix(parseunit, '[') && ParseNumber(parseunit, result.data, true) && matchPrefix(parseunit, ']');
        }
        static bool IsEndChar(const ParseUnit &parseunit) {
            return isEndChar(parseunit.parseinfo, parseunit.currview[0]);
        }
        static bool ParseIdentifier(ParseUnit &parseunit, HashID &result) {
            if (!hasIdentifierPrefix(parseunit.parseinfo, parseunit.currview.get()))
                return false;
            PriLib::StringView::OffsetType begin = isIdentifierEscapePrefixChar(parseunit.parseinfo, parseunit.currview[0]) ? 1 : 0;
            PriLib::StringView::OffsetType end = begin;
            while (isIdentifierChar(parseunit.parseinfo, parseunit.currview[end]))
                ++end;
            PriLib::StringViewRange identstr(parseunit.currview, begin, end);
            result = getGlobalInfo(parseunit.parseinfo).hashStringPool.insert(identstr);
            parseunit.currview += end;
            return false;
        }

        //---------------------------------------------------------------------------------------------
        // * Register
        //---------------------------------------------------------------------------------------------
        template <>
        std::optional<ZeroRegister> Parse<ZeroRegister>(ParseUnit &parseunit) {
            ZeroRegister result;
            if (!matchPrefix(parseunit, "%0"))
                return std::nullopt;
            if (!IsEndChar(parseunit))
                return std::nullopt;
            return result;
        }
        template <>
        std::optional<ResultRegister> Parse<ResultRegister>(ParseUnit &parseunit) {
            ResultRegister result;
            if (!matchPrefix(parseunit, "%res"))
                return std::nullopt;
            if (!IsEndChar(parseunit))
                return std::nullopt;
            return result;
        }
        template <>
        std::optional<DataRegister> Parse<DataRegister>(ParseUnit &parseunit) {
            DataRegister result;
            // Get The First Character
            if (!ParseRegisterScopePrefix(parseunit, result.scopeType))
                return std::nullopt;
            // Get Index
            if (!ParseNumber(parseunit, result.registerIndex.data))
                return std::nullopt;
            if (result.registerIndex.data == 0)
                return std::nullopt;
            // Get Data Register Type
            if (IsEndChar(parseunit)) {
                switch (getGlobalInfo(parseunit.parseinfo).dataRegisterMode) {
                case drm_multiply:
                    return std::nullopt;
                case drm_dynamic:
                    result.registerType = rt_data_dynamic;
                    break;
                case drm_static:
                    result.registerType = rt_data_static;
                    break;
                default:
                    return std::nullopt;
                }
            }
            else {
                switch (parseunit.currview[0]) {
                case 'd':
                    result.registerType = rt_data_dynamic;
                    parseunit.currview += 1;
                    break;
                case 's':
                    result.registerType = rt_data_static;
                    parseunit.currview += 1;
                    break;
                default:
                    return std::nullopt;
                }
                if (!IsEndChar(parseunit))
                    return std::nullopt;
            }
            return result;
        }
        template <>
        std::optional<StackPointerRegister> Parse<StackPointerRegister>(ParseUnit &parseunit) {
            StackPointerRegister result;
            // Get The Prefix
            if (!ParseStackRegisterPrefix(parseunit, result.scopeType))
                return std::nullopt;
            if (!IsEndChar(parseunit)) {
                // Get Offset
                if (!ParseStackOffset(parseunit, result.offset))
                    return std::nullopt;
                if (!IsEndChar(parseunit))
                    return std::nullopt;
            }
            return result;
        }
        template <>
        std::optional<StackSpaceRegister> Parse<StackSpaceRegister>(ParseUnit &parseunit) {
            StackSpaceRegister result;
            // Get The Prefix
            if (!ParseStackRegisterPrefix(parseunit, result.scopeType))
                return std::nullopt;
            if (parseunit.currview[0] == '[') {
                // Get Offset
                if (!ParseStackOffset(parseunit, result.offset))
                    return std::nullopt;
            }
            if (!matchPrefix(parseunit, '('))
                return std::nullopt;
            if (parseunit.currview[0] == ')') {
                result.registerType = rt_stack_space_full;
            }
            else if (isdigit(parseunit.currview[0])) {
                result.registerType = rt_stack_space_size;
                // Get Size
                if (!ParseNumber(parseunit, result.stacksize.data))
                    return std::nullopt;
            }
            else if (ParseIdentifier(parseunit, result.typehashid)) {
                result.registerType = rt_stack_space_type;
            }
            if (!matchPrefix(parseunit, ')'))
                return std::nullopt;
            if (matchPrefix(parseunit, '!')) {
                if (result.offset.data != 0) {
                    return std::nullopt;
                }
                if (result.registerType == rt_stack_space_size)
                    result.registerType = rt_stack_space_size_decrease;
                else if (result.registerType == rt_stack_space_type)
                    result.registerType = rt_stack_space_type_decrease;
                else
                    return std::nullopt;
            }
            if (!IsEndChar(parseunit))
                return std::nullopt;
            return result;
        }
        template <typename RT>
        static std::optional<Register> _parseBase(ParseUnit &parseunit) {
            auto parseresult = Parse<RT>(parseunit);
            return parseresult ? std::optional<Register>(Register(parseresult.value())) : std::nullopt;
        }
        template <>
        std::optional<Register> Parse<Register>(ParseUnit &parseunit) {
            using Func = std::optional<Register> (ParseUnit &parseunit);
            static Func* funcs[] = {
                _parseBase<ZeroRegister>,
                _parseBase<ResultRegister>,
                _parseBase<DataRegister>,
                _parseBase<StackPointerRegister>,
                _parseBase<StackSpaceRegister>
            };
            for (auto func : funcs) {
                ParseUnit record(parseunit);
                auto result = func(record);
                if (result) {
                    parseunit = record;
                    return result;
                }
            }
            return std::nullopt;
        }

        //---------------------------------------------------------------------------------------------
        // * Identifier
        //---------------------------------------------------------------------------------------------
        template <>
        std::optional<Identifier> Parse<Identifier>(ParseUnit &parseunit) {
            HashID hashid;
            if (ParseIdentifier(parseunit, hashid)) {
                return Identifier(hashid);
            }
            return std::nullopt;
        }
    }
}
