#pragma once
#include <string>
#include <utility>
#include <optional>
#include <variant>
#include "config.h"
#include "info.h"
#include "typeinfo.h"

namespace CVM
{
	class ParseInfo;

    namespace InstStructX
    {
        using namespace InstStruct;
        enum RegisterType {
            rt_unknown = 0,

            // Data Register
            rt_data_dynamic,  // %_d (%1d etc.)
            rt_data_static,   // %_s (%1s etc.)

            // Stack Pointer
            //   A stack is a reference to a rt_data_*, like this:
            //     setstr %1d
            //   A stack pointer is a data pointer into stack's memory.
            //   CVM will provide dynamic refer checker for stack pointer.
            rt_stack_pointer,             // %sp, %sp[_]
            rt_stack_space_full,          // %sp()
            rt_stack_space_size,          // %sp(_), %sp[_](_)
            rt_stack_space_type,          // %sp(_), %sp[_](_)
            rt_stack_space_size_decrease, // %sp(_)!, %sp[_](_)!
            rt_stack_space_type_decrease, // %sp(_)!, %sp[_](_)!
        };

        enum RegisterScopeType {
            rst_unknown = 0,
            rst_local,
            rst_thread,
            rst_global,
        };

        DefineExplicitTypeWithValue(RegisterIndex, Config::RegisterIndexType, 0);
        DefineExplicitTypeWithValue(StackOffset, Config::StackOffsetType, 0);
        DefineExplicitTypeWithValue(StackSize, Config::StackSizeType, 0);

        //------------------------------------------------------------------------------------
        // * Data Register
        //    prefix      : {t, g, none}. t means thread, g means global.
        //                  It can be omitted for Local Data Register.
        //    suffix      : {d, s, none}. d means dynamic, s means static.
        //                  It can be omitted if Global Config Mode is not multiply.
        //------------------------------------------------------------------------------------
        struct DataRegister {
            RegisterType registerType = rt_unknown;
            RegisterScopeType scopeType = rst_unknown;
            RegisterIndex registerIndex;

            static std::optional<DataRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw);
            std::string ToString(GlobalInfo &ginfo);
        };

        //------------------------------------------------------------------------------------
        // * Stack Pointer Register
        //    prefix      : {t, g, none}. t means thread, g means global. (%tsp, %gsp, etc.)
        //                  It can be omitted for Local Data Register.
        //    %sp         : Type(cms#pointer), the pointer to stack's first address.
        //    %sp[offset] : Type(cms#pointer), the pointer to (stack + offset).
        //------------------------------------------------------------------------------------
        struct StackPointerRegister {
            RegisterType registerType = rt_unknown;
            RegisterScopeType scopeType = rst_unknown;
            StackOffset offset;

            static std::optional<StackPointerRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw);
            std::string ToString(GlobalInfo &ginfo);
        };

        //------------------------------------------------------------------------------------
        // * Stack Space Register
        //    prefix             : {t, g, none}. t means thread, g means global.
        //                         It can be omitted for Local Data Register.
        //    %sp()              : Type(cms#space), the space of full stack.
        //    %sp(n)             : Type(cms#space), the space of stack[0:n].
        //    %sp(Type)          : Type(Type), the space of stack[0:sizeof(Type)].
        //    %sp[offset](n)     : Type(cms#space), the pointer to (stack + offset)[0:n].
        //    %sp[offset](Type)  : Type(Type), the space of (stack + offset)[0:sizeof(Type)].
        //    %sp[offset](n)!    : Type(cms#space), the pointer to (stack + offset)[0:n],
        //                         decrease after use.
        //    %sp[offset](Type)! : Type(Type), the space of (stack + offset)[0:sizeof(Type)],
        //                         decrease after use.
        //------------------------------------------------------------------------------------
        struct StackSpaceRegister {
            StackSpaceRegister() : stacksize() {}

            RegisterType registerType = rt_unknown;
            RegisterScopeType scopeType = rst_unknown;
            StackOffset offset;
            union {
                StackSize stacksize;
                HashID typehashid;
            };

            static std::optional<StackSpaceRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw);
            std::string ToString(GlobalInfo &ginfo);
        };

        struct Register
        {
            std::variant<DataRegister, StackPointerRegister, StackSpaceRegister> data;
        };
    }
}
