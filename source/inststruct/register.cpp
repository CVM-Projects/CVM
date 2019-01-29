#include "basic.h"
#include "inststruct/register.h"
#include <cctype>

#include "parse.h"

namespace CVM
{
    namespace InstStruct
    {
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
        static std::string ToStringRegisterScopePrefix(RegisterScopeType scopetype) {
            // Set '%t', '%g', '%'
            std::string result;
            // Set The First Character
            result.push_back('%');
            // Set Scope Type
            switch(scopetype) {
            case rst_local:
                break;
            case rst_global:
                result.push_back('g');
                break;
            case rst_thread:
                result.push_back('t');
                break;
            default:
                assert(false && "Invalid Scope Type.");
                break;
            }
            return result;
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
            if (!PriLib::Convert::to_integer<NumType>(std::string(ptr, endptr), result))
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

        //---------------------------------------------------------------------------------------------
        // * Zero Register
        //---------------------------------------------------------------------------------------------
        std::optional<ZeroRegister> ZeroRegister::Parse(ParseUnit &parseunit) {
            ZeroRegister result;
            if (!matchPrefix(parseunit, "%0"))
                return std::nullopt;
            if (!IsEndChar(parseunit))
                return std::nullopt;
            return result;
        }

        std::string ZeroRegister::ToString(GlobalInfo &ginfo) const {
            return "%0";
        }

        //---------------------------------------------------------------------------------------------
        // * Result Register
        //---------------------------------------------------------------------------------------------
        std::optional<ResultRegister> ResultRegister::Parse(ParseUnit &parseunit) {
            ResultRegister result;
            if (!matchPrefix(parseunit, "%res"))
                return std::nullopt;
            if (!IsEndChar(parseunit))
                return std::nullopt;
            return result;
        }

        std::string ResultRegister::ToString(GlobalInfo &ginfo) const {
            return "%res";
        }

        //---------------------------------------------------------------------------------------------
        // * Data Register
        //---------------------------------------------------------------------------------------------
        std::optional<DataRegister> DataRegister::Parse(ParseUnit &parseunit) {
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

        std::string DataRegister::ToString(GlobalInfo &ginfo) const {
            std::string result;
            // Set Scope Prefix
            result += ToStringRegisterScopePrefix(this->scopeType);
            // Set Index
            result += to_string(this->registerIndex.data);
            // Set Data Register Type
            if (ginfo.dataRegisterMode == drm_multiply) {
                switch (this->registerType) {
                case rt_data_dynamic:
                    result.push_back('d');
                    break;
                case rt_data_static:
                    result.push_back('s');
                    break;
                default:
                    assert(false && "Invalid data for DataRegister.");
                    break;
                }
            }
            return result;
        }

        //---------------------------------------------------------------------------------------------
        // * Stack Pointer Register
        //---------------------------------------------------------------------------------------------
        std::optional<StackPointerRegister> StackPointerRegister::Parse(ParseUnit &parseunit) {
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

        std::string StackPointerRegister::ToString(GlobalInfo &ginfo) const {
            std::string result;
            // Set Prefix
            result += ToStringRegisterScopePrefix(this->scopeType);
            result += "sp";
            // Set Offset
            if (this->offset.data != 0) {
                result += "[";
                result += to_string(this->offset.data);
                result += "]";
            }
            return result;
        }

        //---------------------------------------------------------------------------------------------
        // * Stack Space Register
        //---------------------------------------------------------------------------------------------
        std::optional<StackSpaceRegister> StackSpaceRegister::Parse(ParseUnit &parseunit) {
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
            else if (hasIdentifierPrefix(parseunit.parseinfo, parseunit.currview.get())) {
                result.registerType = rt_stack_space_type;
                // Get Type
                const char *endptr = parseunit.currview.get();
                while (isIdentifierChar(parseunit.parseinfo, *endptr))
                    ++endptr;
                result.typehashid = getGlobalInfo(parseunit.parseinfo).hashStringPool.insert(std::string(parseunit.currview.get(), endptr));
                parseunit.currview = PriLib::StringView(endptr);
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

        std::string StackSpaceRegister::ToString(GlobalInfo &ginfo) const {
            std::string result;
            // Set Prefix
            result += ToStringRegisterScopePrefix(this->scopeType);
            result += "sp";
            // Set Offset
            if (this->offset.data != 0) {
                result += "[";
                result += to_string(this->offset.data);
                result += "]";
            }
            // Set Space
            result += "(";
            if (this->registerType != rt_stack_space_full) {
                if (this->registerType == rt_stack_space_size || this->registerType == rt_stack_space_size_decrease) {
                    result += to_string(this->stacksize.data);
                }
                else if (this->registerType == rt_stack_space_type || this->registerType == rt_stack_space_type_decrease) {
                    result += ginfo.hashStringPool.get(this->typehashid);
                }
            }
            result += ")";
            if (this->registerType == rt_stack_space_size_decrease || this->registerType == rt_stack_space_type_decrease) {
                result += "!";
            }
            return result;
        }
    }
}
