#pragma once
#include <string>
#include <utility>
#include <optional>
#include <variant>
#include "config.h"
#include "info.h"

namespace CVM
{
	class ParseInfo;

    namespace InstStructX
    {
        using namespace InstStruct;
        //------------------------------------------------------------------------------------
        // * Types
        //------------------------------------------------------------------------------------
        enum RegisterType : uint8_t {
            rt_unknown = 0,

            // Zero Register
            rt_zero,

            // Result Register
            rt_result,

            // Data Register
            rt_data_dynamic,  // %_d (%1d etc.)
            rt_data_static,   // %_s (%1s etc.)

            // Stack Pointer
            //   A stack is a reference to a rt_data_*, like this:
            //     set %sp %1d
            //   A stack pointer is a data pointer into stack's memory.
            //   CVM will provide dynamic refer checker for stack pointer.
            rt_stack_pointer,             // %sp, %sp[_]
            rt_stack_space_full,          // %sp()
            rt_stack_space_size,          // %sp(_), %sp[_](_)
            rt_stack_space_type,          // %sp(_), %sp[_](_)
            rt_stack_space_size_decrease, // %sp(_)!, %sp[_](_)!
            rt_stack_space_type_decrease, // %sp(_)!, %sp[_](_)!
        };

        inline bool is_rt_data(RegisterType rtype) {
            return rt_data_dynamic <= rtype && rtype <= rt_data_static;
        }
        inline bool is_rt_stack(RegisterType rtype) {
            return rt_stack_pointer <= rtype && rtype <= rt_stack_space_type_decrease;
        }
        inline bool is_rt_stack_pointer(RegisterType rtype) {
            return rtype == rt_stack_pointer;
        }
        inline bool is_rt_stack_space(RegisterType rtype) {
            return rt_stack_space_full <= rtype && rtype <= rt_stack_space_type_decrease;
        }

        enum RegisterScopeType : uint8_t {
            rst_unknown = 0,
            rst_local,
            rst_thread,
            rst_global,
        };

        DefineExplicitTypeWithValue(RegisterIndex, Config::RegisterIndexType, 0);
        DefineExplicitTypeWithValue(StackOffset, Config::StackOffsetType, 0);
        DefineExplicitTypeWithValue(StackSize, Config::StackSizeType, 0);

        //------------------------------------------------------------------------------------
        // * Register Info
        //------------------------------------------------------------------------------------
        struct RegisterInfo {
            RegisterInfo() = default;
            RegisterInfo(RegisterType registerType, RegisterScopeType scopeType)
                : registerType(registerType), scopeType(scopeType) {}

            RegisterType registerType = rt_unknown;
            RegisterScopeType scopeType = rst_unknown;
        };

        //------------------------------------------------------------------------------------
        // * Zero Register
        //    %0
        //------------------------------------------------------------------------------------
        struct ZeroRegister;
        struct ZeroRegisterBase {
            using ImplType = ZeroRegister;
        };
        struct ZeroRegister : public RegisterInfo, public ZeroRegisterBase {
            using BaseType = ZeroRegisterBase;

            ZeroRegister() : RegisterInfo(rt_zero, rst_local) {}

            static std::optional<ZeroRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr);
            std::string ToString(GlobalInfo &ginfo) const;
        };

        //------------------------------------------------------------------------------------
        // * Result Register
        //    %res
        //------------------------------------------------------------------------------------
        struct ResultRegister;
        struct ResultRegisterBase {
            using ImplType = ResultRegister;
        };
        struct ResultRegister : public RegisterInfo, public ResultRegisterBase {
            using BaseType = ResultRegisterBase;

            ResultRegister() : RegisterInfo(rt_result, rst_local) {}

            static std::optional<ResultRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr);
            std::string ToString(GlobalInfo &ginfo) const;
        };

        //------------------------------------------------------------------------------------
        // * Data Register
        //    prefix      : {t, g, none}. t means thread, g means global.
        //                  It can be omitted for Local Data Register.
        //    suffix      : {d, s, none}. d means dynamic, s means static.
        //                  It can be omitted if Global Config Mode is not multiply.
        //------------------------------------------------------------------------------------
        struct DataRegister;
        struct DataRegisterBase {
            using ImplType = DataRegister;

            RegisterIndex registerIndex;
        };
        struct DataRegister : public RegisterInfo, public DataRegisterBase {
            using BaseType = DataRegisterBase;

            static std::optional<DataRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr);
            std::string ToString(GlobalInfo &ginfo) const;
        };

        //------------------------------------------------------------------------------------
        // * Stack Pointer Register
        //    prefix      : {t, g, none}. t means thread, g means global. (%tsp, %gsp, etc.)
        //                  It can be omitted for Local Data Register.
        //    %sp         : Type(cms#pointer), the pointer to stack's first address.
        //    %sp[offset] : Type(cms#pointer), the pointer to (stack + offset).
        //------------------------------------------------------------------------------------
        struct StackPointerRegister;
        struct StackPointerRegisterBase {
            using ImplType = StackPointerRegister;

            StackOffset offset;
        };
        struct StackPointerRegister : public RegisterInfo, public StackPointerRegisterBase {
            using BaseType = StackPointerRegisterBase;

            StackPointerRegister() : RegisterInfo(rt_stack_pointer,rst_unknown) {}

            static std::optional<StackPointerRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr);
            std::string ToString(GlobalInfo &ginfo) const;
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
        struct StackSpaceRegister;
        struct StackSpaceRegisterBase {
            using ImplType = StackSpaceRegister;

            StackSpaceRegisterBase() : stacksize() {}
            StackOffset offset;
            union {
                StackSize stacksize;
                HashID typehashid;
            };
        };
        struct StackSpaceRegister : public RegisterInfo, public StackSpaceRegisterBase {
            using BaseType = StackSpaceRegisterBase;

            static std::optional<StackSpaceRegister> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr);
            std::string ToString(GlobalInfo &ginfo) const;
        };

        //------------------------------------------------------------------------------------
        // * Register
        //------------------------------------------------------------------------------------
        struct Register : public RegisterInfo {
            std::variant<ZeroRegisterBase, ResultRegisterBase, DataRegisterBase, StackPointerRegisterBase, StackSpaceRegisterBase> data;

            Register() = default;

            template <typename RT>
            Register(const RT &value)
                : RegisterInfo(value), data(value) {}

            template <typename RT>
            Register& operator=(const RT &value) {
                this->registerType = value.registerType;
                this->scopeType = value.scopeType;
                this->data = static_cast<typename RT::BaseType>(value);
                return *this;
            }

            static std::optional<Register> Parse(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr) {
                using Func = std::optional<Register> (ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize);
                static Func* funcs[] = {
                    _parseBase<ZeroRegister>,
                    _parseBase<ResultRegister>,
                    _parseBase<DataRegister>,
                    _parseBase<StackPointerRegister>,
                    _parseBase<StackSpaceRegister>
                };
                for (auto func : funcs) {
                    auto result = func(parseinfo, raw, matchsize);
                    if (result)
                        return result;
                }
                return std::nullopt;
            }

            std::string ToString(GlobalInfo &ginfo) const {
                if (this->registerType == rt_zero)
                    return _toString<ZeroRegister>(ginfo);
                else if (this->registerType == rt_result)
                    return _toString<ResultRegister>(ginfo);
                else if (is_rt_data(this->registerType))
                    return _toString<DataRegister>(ginfo);
                else if (is_rt_stack_pointer(this->registerType))
                    return _toString<StackPointerRegister>(ginfo);
                else if (is_rt_stack_space(this->registerType))
                    return _toString<StackSpaceRegister>(ginfo);
                else {
                    assert(false && "Invalid data for Register.");
                    return "";
                }
            }

        private:
            template <typename RT>
            static std::optional<Register> _parseBase(ParseInfo &parseinfo, const PriLib::StringView &raw, ptrdiff_t *matchsize = nullptr) {
                auto parseresult = RT::Parse(parseinfo, raw, matchsize);
                if (parseresult)
                    return Register(parseresult.value());
                else
                    return std::nullopt;
            }
            template <typename RBT>
            static std::string _toString(const RBT &registerbasetype, GlobalInfo &ginfo, const RegisterInfo &rinfo) {
                typename RBT::ImplType impl;
                static_cast<RegisterInfo&>(impl) = rinfo;
                static_cast<RBT&>(impl) = registerbasetype;
                return impl.ToString(ginfo);
            }
            template <typename RT>
            std::string _toString(GlobalInfo &ginfo) const {
                return _toString(std::get<typename RT::BaseType>(data), ginfo, _rinfo());
            }
            const RegisterInfo& _rinfo() const {
                return static_cast<const RegisterInfo&>(*this);
            }
        };
    }
}
