#include "basic.h"
#include "inststruct/register.h"
#include <cctype>

#include "parser/parse.h"

namespace CVM
{
    namespace InstStruct
    {
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

        //---------------------------------------------------------------------------------------------
        // * Zero Register
        //---------------------------------------------------------------------------------------------
        std::string ZeroRegister::ToString(GlobalInfo &ginfo) const {
            return "%0";
        }

        //---------------------------------------------------------------------------------------------
        // * Result Register
        //---------------------------------------------------------------------------------------------
        std::string ResultRegister::ToString(GlobalInfo &ginfo) const {
            return "%res";
        }

        //---------------------------------------------------------------------------------------------
        // * Data Register
        //---------------------------------------------------------------------------------------------
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
