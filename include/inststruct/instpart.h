#pragma once
#include <cstdint>
#include <cassert>
#include <string>
#include <bignumber.h>
#include "config.h"
#include "typeinfo.h"
#include "register.h"
#include "instpart-enum.h"

namespace CVM
{
	namespace InstStruct
	{
		struct Identifier {
			using Type = HashID;
			static constexpr ElementType elementType = ET_Identifier;

			explicit Identifier(const Type &data) : _data(data) {}
			const Type& data() const { return _data; }

		private:
			Type _data;
		};

		struct String {
			using Type = std::string;
			static constexpr ElementType elementType = ET_String;

			explicit String(Type data) : _data(std::move(data)) {}
			const Type& data() const { return _data; }

		private:
			Type _data;
		};

		struct DataLabel {
			using Type = Config::DataIndexType;
			static constexpr ElementType elementType = ET_DataLabel;

			explicit DataLabel(Type data) : _data(data) {}
			const Type& data() const { return _data; }

		private:
			Type _data;
		};

		struct LineLabel {
			using Type = HashID;
			static constexpr ElementType elementType = ET_LineLabel;

			explicit LineLabel(const Type &data) : _data(data) {}
			const Type& data() const { return _data; }

		private:
			Type _data;
		};

		struct ArrayData {
			using Type = std::vector<uint8_t>;
			static constexpr ElementType elementType = ET_ArrayData;

			explicit ArrayData(Type data) : _data(std::move(data)) {}
			const Type& data() const { return _data; }

		private:
			Type _data;
		};

		struct IntegerData {
			using Type = CVM::BigInteger;
			static constexpr ElementType elementType = ET_IntegerData;

			explicit IntegerData(Type data, bool has_signed_prefix)
				: _data(std::move(data)), _has_signed_prefix(has_signed_prefix) {}
			const Type& data() const { return _data; }

			bool has_signed_prefix() const { return _has_signed_prefix; }
			bool is_negative() const {
				return _has_signed_prefix && _data.is_negative();
			}

		private:
			Type _data;
			bool _has_signed_prefix;  // If write signed ('+' or '-') prefix
		};

		struct Element {
#define InstPart(key) explicit Element(key data) : _type(ET_##key), _data(std::move(data)) {}
#include "instpart.def"

			template <typename T>
			const T& get() const {
				return std::get<T>(_data);
			}
			ElementType type() const {
				return _type;
			}

		private:
			ElementType _type;
			std::variant<
#define InstPart(key) key,
#include "instpart.def"
			void*
			> _data;
		};

		struct FuncIdentifier
		{
		public:
			using Type = Config::FuncIndexType;

			explicit FuncIdentifier(Type index)
				: _data(index) {}

			Type data() const {
				return _data;
			}
//
//			std::string toString() const {
//				return "i" + std::to_string(_data);
//			}

		private:
			Type _data;
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
//
//			std::string toString() const {
//				return "0x" + PriLib::Convert::to_hex(_data);
//			}

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

//			std::string ToString(GlobalInfo &ginfo) const {
//				std::string result;
//				result += "[";
//				for (auto &dat : _data) {
//					result += dat.ToString(ginfo) + ", ";
//				}
//				result.pop_back();
//				result.pop_back();
//				result += "]";
//				return result;
//			}

		private:
			Type _data;
		};

		template <typename T>
		std::string ToString(const T &data, GlobalInfo &ginfo);
	}
}
