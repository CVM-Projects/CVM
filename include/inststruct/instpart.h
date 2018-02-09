#pragma once
#include <cstdint>
#include <cassert>
#include <string>
#include <bitset>
#include "../prilib/include/prints.h"
#include "config.h"

namespace CVM
{
	namespace InstStruct
	{
		enum RegisterType : uint8_t {
			r_n = 0,   // %_
			r_t,       // %t_
			r_g,       // %g_
			r_res,     // %res
			r_sp,      // %sp
			r_spt,     // %sp[x](Type)  %sp(Type) = %sp[0](Type)
			r_spn,     // %sp[x](n)     %sp(n)    = %sp[0](n)
			r_sptd,    // %sp(Type)!
			r_spnd,    // %sp(n)!
		};

		enum EnvType : uint8_t {
			e_current = 0,  // %env
			e_parent,       // %penv
			e_temp,         // %tenv
		};

		struct Register
		{
		public:
			using RegisterIndexType = Config::RegisterIndexType;

		public:
			static Register ZeroRegister() {
				return Register(r_n);
			}
			static Register ResultRegister() {
				return Register(r_res);
			}
			static Register PrivateDataRegister(Config::RegisterIndexType rindex, EnvType etype) {
				Register r(r_n);
				r._etype = etype;
				r._rindex = rindex;
				return r;
			}
			static Register ThreadDataRegister(Config::RegisterIndexType rindex) {
				Register r(r_t);
				r._rindex = rindex;
				return r;
			}
			static Register GlobalDataRegister(Config::RegisterIndexType rindex) {
				Register r(r_g);
				r._rindex = rindex;
				return r;
			}
			static Register StackPointerRegister() {
				return Register(r_sp);
			}
			static Register StackRegisterType(TypeIndex typeindex, Config::StackOffsetType offset) {
				Register r(r_spt);
				r._tindex = typeindex.data;
				r._spoff = offset;
				return r;
			}
			static Register StackRegisterSize(MemorySize memsize, Config::StackOffsetType offset) {
				Register r(r_spn);
				r._msize = memsize.data;
				r._spoff = offset;
				return r;
			}
			static Register StackRegisterTypeDecrease(TypeIndex typeindex) {
				Register r(r_sptd);
				r._tindex = typeindex.data;
				return r;
			}
			static Register StackRegisterSizeDecrease(MemorySize memsize) {
				Register r(r_spnd);
				r._msize = memsize.data;
				return r;
			}

		public:
			explicit Register(RegisterType type = r_n)
				: _type(type), _etype(e_current), _rindex(0) {}

			RegisterType type() const {
				return _type;
			}
			EnvType etype() const {
				assert(_type == r_n && _rindex != 0);
				return EnvType(_etype);
			}
			RegisterIndexType index() const {
				assert(_type == r_n);
				return _rindex;
			}

			bool check() const {
				if (_type == r_spt || _type == r_spn || _type == r_sptd || _type == r_spnd) {

				}
				// Only %n (n > 0) support env.
				else if ((_type != r_n || isZeroRegister()) && _etype != e_current)
					return false;
				return true;
			}

			bool isResultRegister() const {
				return _type == r_res;
			}
			bool isZeroRegister() const {
				return _type == r_n && _rindex == 0;
			}
			bool isPrivateDataRegister() const {
				return _type == r_n && _etype == e_current && _rindex > 0;
			}

			std::string toString() const {
				assert(check());
				std::string result;
				switch (_type) {
				case r_n: result += "%" + std::to_string(_rindex); break;
				case r_t: result += "%t" + std::to_string(_rindex); break;
				case r_g: result += "%g" + std::to_string(_rindex); break;
				case r_res: result += "%res"; break;
				default:  result += "%?"; break;
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
			RegisterType _type = r_n;
			union {
				struct {
					EnvType _etype;
					RegisterIndexType _rindex;
				};
				struct {
					Config::StackOffsetType _spoff;
					union {
						Config::TypeIndexType _tindex; // r_spt
						Config::MemorySizeType _msize; // r_spn
					};
				};
			};
		};

		struct Identifier
		{
		public:
			using Type = uint32_t;

			explicit Identifier(Type index)
				: _index(index) {}

			Type index() const {
				return _index;
			}

			std::string toString() const {
				return "i" + std::to_string(_index);
			}

		private:
			Type _index;
		};

		struct DataIndex
		{
		public:
			using Type = Config::DataIndexType;

			explicit DataIndex(Type index)
				: _index(index) {}

			Type index() const {
				return _index;
			}

			std::string toString() const {
				return "#" + std::to_string(_index);
			}

		private:
			Type _index;
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
