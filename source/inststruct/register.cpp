#include "basic.h"
#include "inststruct/register.h"
#include <cctype>

#include "parse.h"

namespace CVM
{
    namespace InstStructX
    {
        static bool ParseRegisterPrefix(ParseInfo &parseinfo, const PriLib::StringView &raw, const char* &outptr) {
            // Parse '%'
            outptr = raw.get();
            if (*outptr != '%') return false;
            outptr = outptr + 1;
            return true;
        }
        static bool ParseRegisterScopePrefix(ParseInfo &parseinfo, const PriLib::StringView &raw, const char* &outptr, RegisterScopeType &scopetype) {
            // Parse '%t', '%g', '%'
            if (!ParseRegisterPrefix(parseinfo, raw, outptr))
                return false;
            switch (*outptr) {
            case 't': scopetype = rst_thread; outptr = outptr + 1; break;
            case 'g': scopetype = rst_global; outptr = outptr + 1; break;
            default: scopetype = rst_local; break;
            }
            return true;
        }
        static bool ParseStackRegisterPrefix(ParseInfo &parseinfo, const PriLib::StringView &raw, const char* &outptr, RegisterScopeType &scopetype) {
            // Parse '%tsp', '%gsp', '%sp'
            if (!ParseRegisterScopePrefix(parseinfo, raw, outptr, scopetype))
                return false;
            if (outptr[0] != 's' || outptr[1] != 'p') return false;
            outptr = outptr + 2;
            return true;
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
        static bool ParseNumber(ParseInfo &parseinfo, const char* &outptr, NumType &result, bool allow_negative = false) {
            const char *endptr = outptr;
            if (allow_negative && (*outptr == '-') || (*outptr == '+'))
                ++endptr;
            while (isdigit(*endptr))
                ++endptr;
            if (outptr == endptr)
                return false;
            if (!PriLib::Convert::to_integer<NumType>(std::string(outptr, endptr), result))
                return false;
            outptr = endptr;
            return true;
        }
        static bool ParseStackOffset(ParseInfo &parseinfo, const char* &outptr, StackOffset &result) {
            if (*outptr != '[')
                return false;
            outptr = outptr + 1;
            if (!ParseNumber(parseinfo, outptr, result.data, true))
                return false;
            if (*outptr != ']')
                return false;
            outptr = outptr + 1;
            return true;
        }

        //------------------------------------------------------------------------------------
        // * Zero Register
        //------------------------------------------------------------------------------------
        std::optional<ZeroRegister> ZeroRegister::Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize) {
            ZeroRegister result;
            const char *ptr;
            if (!ParseRegisterPrefix(parseinfo, raw, ptr))
                return std::nullopt;
            if (*ptr != '0')
                return std::nullopt;
            ptr = ptr + 1;
            if (!isEndChar(parseinfo, *ptr))
                return std::nullopt;
            if (matchsize)
                *matchsize = ptr - raw.get();
            return result;
        }

        std::string ZeroRegister::ToString(GlobalInfo &ginfo) const {
            return "%0";
        }

        //------------------------------------------------------------------------------------
        // * Result Register
        //------------------------------------------------------------------------------------
        std::optional<ResultRegister> ResultRegister::Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize) {
            ResultRegister result;
            const char *ptr;
            if (!ParseRegisterPrefix(parseinfo, raw, ptr))
                return std::nullopt;
            if (ptr[0] != 'r' || ptr[1] != 'e' || ptr[2] != 's')
                return std::nullopt;
            ptr = ptr + 3;
            if (!isEndChar(parseinfo, *ptr))
                return std::nullopt;
            if (matchsize)
                *matchsize = ptr - raw.get();
            return result;
        }

        std::string ResultRegister::ToString(GlobalInfo &ginfo) const {
            return "%res";
        }

        //------------------------------------------------------------------------------------
        // * Data Register
        //------------------------------------------------------------------------------------
        std::optional<DataRegister> DataRegister::Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize) {
            DataRegister result;
            // Get The First Character
            const char *ptr = nullptr;
            if (!ParseRegisterScopePrefix(parseinfo, raw, ptr, result.scopeType))
                return std::nullopt;
            // Get Index
            if (!ParseNumber(parseinfo, ptr, result.registerIndex.data))
                return std::nullopt;
            if (result.registerIndex.data == 0)
                return std::nullopt;
            // Get Data Register Type
            if (isEndChar(parseinfo, *ptr)) {
                switch (getGlobalInfo(parseinfo).dataRegisterMode) {
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
                switch (*ptr) {
                case 'd':
                    result.registerType = rt_data_dynamic;
                    break;
                case 's':
                    result.registerType = rt_data_static;
                    break;
                default:
                    return std::nullopt;
                }
                if (!isEndChar(parseinfo, ptr[1]))
                    return std::nullopt;
            }
            if (matchsize)
                *matchsize = ptr - raw.get();
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

        //------------------------------------------------------------------------------------
        // * Stack Pointer Register
        //------------------------------------------------------------------------------------
        std::optional<StackPointerRegister> StackPointerRegister::Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize) {
            StackPointerRegister result;
            // Get The Prefix
            const char *ptr = nullptr;
            if (!ParseStackRegisterPrefix(parseinfo, raw, ptr, result.scopeType))
                return std::nullopt;
            if (!isEndChar(parseinfo, *ptr)) {
                // Get Offset
                if (!ParseStackOffset(parseinfo, ptr, result.offset))
                    return std::nullopt;
                if (!isEndChar(parseinfo, *ptr))
                    return std::nullopt;
            }
            if (matchsize)
                *matchsize = ptr - raw.get();
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

        //------------------------------------------------------------------------------------
        // * Stack Space Register
        //------------------------------------------------------------------------------------
        std::optional<StackSpaceRegister> StackSpaceRegister::Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize) {
            StackSpaceRegister result;
            // Get The Prefix
            const char *ptr = nullptr;
            if (!ParseStackRegisterPrefix(parseinfo, raw, ptr, result.scopeType))
                return std::nullopt;
            if (*ptr == '[') {
                // Get Offset
                if (!ParseStackOffset(parseinfo, ptr, result.offset))
                    return std::nullopt;
            }
            if (*ptr != '(')
                return std::nullopt;
            ptr = ptr + 1;
            if (*ptr == ')') {
                result.registerType = rt_stack_space_full;
            }
            else if (isdigit(*ptr)) {
                result.registerType = rt_stack_space_size;
                // Get Size
                if (!ParseNumber(parseinfo, ptr, result.stacksize.data))
                    return std::nullopt;
            }
            else if (hasIdentifierPrefix(parseinfo, ptr)) {
                result.registerType = rt_stack_space_type;
                // Get Type
                const char *endptr = ptr;
                while (isIdentifierChar(parseinfo, *endptr))
                    ++endptr;
                result.typehashid = getGlobalInfo(parseinfo).hashStringPool.insert(std::string(ptr, endptr));
                ptr = endptr;
            }
            if (*ptr != ')')
                return std::nullopt;
            ptr = ptr + 1;
            if (*ptr == '!') {
                if (result.registerType == rt_stack_space_size)
                    result.registerType = rt_stack_space_size_decrease;
                else if (result.registerType == rt_stack_space_type)
                    result.registerType = rt_stack_space_type_decrease;
                else
                    return std::nullopt;
                ptr = ptr + 1;
            }
            if (!isEndChar(parseinfo, *ptr))
                return std::nullopt;
            if (matchsize)
                *matchsize = ptr - raw.get();
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
