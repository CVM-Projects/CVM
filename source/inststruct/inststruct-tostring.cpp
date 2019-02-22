#include "basic.h"
#include "inststruct/instpart.h"
#include <cctype>

#include "parser/parse.h"

namespace CVM
{
	namespace InstStruct
	{
		//---------------------------------------------------------------------------------------------
		// * Register
		//---------------------------------------------------------------------------------------------
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

		template <>
		std::string ToString<ZeroRegister>(const ZeroRegister &data, GlobalInfo &ginfo) {
			return "%0";
		}

		template <>
		std::string ToString<ResultRegister>(const ResultRegister &data, GlobalInfo &ginfo) {
			return "%res";
		}

		template <>
		std::string ToString<DataRegister>(const DataRegister &data, GlobalInfo &ginfo) {
			std::string result;
			// Set Scope Prefix
			result += ToStringRegisterScopePrefix(data.scopeType);
			// Set Index
			result += to_string(data.registerIndex.data);
			// Set Data Register Type
			if (ginfo.dataRegisterMode == drm_multiply) {
				switch (data.registerType) {
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

		template <>
		std::string ToString<StackPointerRegister>(const StackPointerRegister &data, GlobalInfo &ginfo) {
			std::string result;
			// Set Prefix
			result += ToStringRegisterScopePrefix(data.scopeType);
			result += "sp";
			// Set Offset
			if (data.offset.data != 0) {
				result += "[";
				result += to_string(data.offset.data);
				result += "]";
			}
			return result;
		}

		template <>
		std::string ToString<StackSpaceRegister>(const StackSpaceRegister &data, GlobalInfo &ginfo) {
			std::string result;
			// Set Prefix
			result += ToStringRegisterScopePrefix(data.scopeType);
			result += "sp";
			// Set Offset
			if (data.offset.data != 0) {
				result += "[";
				result += to_string(data.offset.data);
				result += "]";
			}
			// Set Space
			result += "(";
			if (data.registerType != rt_stack_space_full) {
				if (data.registerType == rt_stack_space_size || data.registerType == rt_stack_space_size_decrease) {
					result += to_string(data.stacksize.data);
				}
				else if (data.registerType == rt_stack_space_type || data.registerType == rt_stack_space_type_decrease) {
					result += ginfo.hashStringPool.get(data.typehashid);
				}
			}
			result += ")";
			if (data.registerType == rt_stack_space_size_decrease || data.registerType == rt_stack_space_type_decrease) {
				result += "!";
			}
			return result;
		}

		template <typename RT>
		static std::string _toString(const Register &data, GlobalInfo &ginfo) {
			RT impl;
			static_cast<RegisterInfo&>(impl) = static_cast<const RegisterInfo&>(data);
			static_cast<typename RT::BaseType&>(impl) = std::get<typename RT::BaseType>(data.data);
			return InstStruct::ToString(impl, ginfo);
		}
		template <>
		std::string ToString<Register>(const Register &data, GlobalInfo &ginfo) {
			if (data.registerType == rt_zero)
				return _toString<ZeroRegister>(data, ginfo);
			else if (data.registerType == rt_result)
				return _toString<ResultRegister>(data, ginfo);
			else if (is_rt_data(data.registerType))
				return _toString<DataRegister>(data, ginfo);
			else if (is_rt_stack_pointer(data.registerType))
				return _toString<StackPointerRegister>(data, ginfo);
			else if (is_rt_stack_space(data.registerType))
				return _toString<StackSpaceRegister>(data, ginfo);
			else {
				assert(false && "Invalid data for Register.");
				return "";
			}
		}

		//---------------------------------------------------------------------------------------------
		// * Identifier
		//---------------------------------------------------------------------------------------------
		template <>
		std::string ToString<Identifier>(const Identifier &data, GlobalInfo &ginfo) {
			return ginfo.hashStringPool.get(data.data());
		}

		//---------------------------------------------------------------------------------------------
		// * String
		//---------------------------------------------------------------------------------------------
		template <>
		std::string ToString<String>(const String &data, GlobalInfo &ginfo) {
			std::string result;
			result += '"';
			for (auto c : data.data()) {
				switch (c) {
				case '"': result += "\\\""; break;
				case '\\': result += "\\\\"; break;
				case '\a': result += "\\a"; break;
				case '\b': result += "\\b"; break;
				case '\f': result += "\\f"; break;
				case '\n': result += "\\n"; break;
				case '\r': result += "\\r"; break;
				case '\t': result += "\\t"; break;
				case '\v': result += "\\v"; break;
				default:
					if (std::isprint(c)) {
						result += c;
					} else {
						result += "\\x" + PriLib::Convert::to_hex(static_cast<uint8_t >(c));
					}
				}
			}
			result += '"';
			return result;
		}

		//---------------------------------------------------------------------------------------------
		// * DataLabel
		//---------------------------------------------------------------------------------------------
		template <>
		std::string ToString<DataLabel>(const DataLabel &data, GlobalInfo &ginfo) {
			return "#" + std::to_string(data.data());
		}

		//---------------------------------------------------------------------------------------------
		// * LineLabel
		//---------------------------------------------------------------------------------------------
		template <>
		std::string ToString<LineLabel>(const LineLabel &data, GlobalInfo &ginfo) {
			return "#" + ginfo.hashStringPool.get(data.data());
		}
	}
}
