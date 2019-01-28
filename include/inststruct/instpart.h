#pragma once
#include <cstdint>
#include <cassert>
#include <string>
#include <bitset>
#include "../prilib/include/prints.h"
#include "config.h"
#include "typeinfo.h"
#include "register.h"

namespace CVM
{
	class ParseInfo;

	namespace InstStruct
	{
		struct FuncIdentifier
		{
		public:
			using Type = Config::FuncIndexType;

			explicit FuncIdentifier(Type index)
				: _data(index) {}

			Type data() const {
				return _data;
			}

			std::string toString() const {
				return "i" + std::to_string(_data);
			}

		private:
			Type _data;
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

		struct ArgumentList
		{
		public:
			using Type = PriLib::lightlist<Register>;
			using Creater = PriLib::lightlist_creater<Register>;

			ArgumentList(const Type &data)
				: _data(data) {}

			const Type& data() const {
				return _data;
			}

			std::string toString() const {
				std::string result;
				result += "[";
				for (auto &dat : _data) {
					result += dat.toString() + ", ";
				}
				result.pop_back();
				result.pop_back();
				result += "]";
				return result;
			}

		private:
			Type _data;
		};
	}
}
