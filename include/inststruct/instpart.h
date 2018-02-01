#pragma once
#include <cstdint>
#include <cassert>
#include <string>
#include <bitset>
#include "../prilib/include/prints.h"

namespace CVM
{
	namespace InstStruct
	{
		enum RegisterType : uint8_t {
			r_n = 0,   // %_
			r_t = 1,   // %t_
			r_g = 2,   // %g_
			r_x = 3,   // %res(-1)
			r_0,       // %0   = %_(0)
			r_res,     // %res = %x(-1)
		};

		enum EnvType : uint8_t {
			e_current = 0,  // %env
			e_parent = 1,   // %penv
			e_temp = 2,     // %tenv
		};

		struct Register
		{
		public:
			explicit Register()
				: _value(0) {}

			explicit Register(RegisterType type)
				: _type(type) {
				if (type == r_0) {
					_value = 0;
				}
				else if (type == r_res) {
					_value = UINT32_MAX;
				}
				else {
					assert(false);
				}
			}

			explicit Register(RegisterType type, EnvType etype, uint16_t index)
				: _type(type), _etype(etype), _index(index) {
				assert(index <= UINT16_MAX);
				if (type == r_0) {
					_value = 0;
				}
				else if (type == r_res) {
					_value = UINT32_MAX;
				}
			}

			RegisterType type() const {
				return RegisterType(_type);
			}
			EnvType etype() const {
				return EnvType(_etype);
			}
			uint16_t index() const {
				return _index;
			}

			bool isResRegister() const {
				return _value == UINT32_MAX;
			}
			bool isZeroRegister() const {
				return _value == 0;
			}

			std::string toString() const {
				std::string result;
				switch (_type) {
				case r_n: result += "%" + std::to_string(_index); break;
				case r_t: result += "%t" + std::to_string(_index); break;
				case r_g: result += "%g" + std::to_string(_index); break;
				default:  result += isResRegister() ? "%res" : "%?"; break;
				}
				switch (_etype) {
				case e_current: break;
				case e_parent: result += "(%penv)"; break;
				case e_temp: result += "(%tenv)"; break;
				default: result += "(%?)"; break;
				}
				return result;
			}

		private:
			union {
				struct {
					RegisterType _type;
					EnvType _etype;
					uint16_t _index;
				};
				uint32_t _value;
			};
		};

		struct Identifier
		{
		public:
			explicit Identifier(uint32_t index)
				: _index(index) {}

			uint32_t index() const {
				return _index;
			}

			std::string toString() const {
				return "i" + std::to_string(_index);
			}

		private:
			uint32_t _index;
		};

		struct DataIndex
		{
		public:
			explicit DataIndex(uint32_t index)
				: _index(index) {}

			uint32_t index() const {
				return _index;
			}

			std::string toString() const {
				return "d" + std::to_string(_index);
			}

		private:
			uint32_t _index;
		};

		struct Data
		{
		public:
			using Type = uint32_t;

			explicit Data(Type data)
				: _data(data) {}

			Type data() const {
				return _data;
			}

			std::string toString() const {
				return "0x" + PriLib::Convert::to_hex(_data);
			}

		private:
			Type _data;
		};
	}
}
