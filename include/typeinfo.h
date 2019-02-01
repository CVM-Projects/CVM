#pragma once
#include "../prilib/include/explicittype.h"
#include <cstdint>
#include <map>
#include <vector>
#include <limits>
#include "config.h"
#include "typebase.h"
#include "inststruct/hashstringpool.h"

namespace CVM
{
	DefineExplicitTypeWithValue(TypeIndex, Config::TypeIndexType, 0);
	struct TypeIndexLess {
		bool operator()(const TypeIndex &t1, const TypeIndex &t2) const {
			return t1.data < t2.data;
		}
	};

	DefineExplicitTypeWithValue(MemoryCount, Config::MemoryCountType, 0);

	class MemorySize : public PriLib::ExplicitType<Config::MemorySizeType, 0>
	{
	public:
		explicit MemorySize(Config::MemorySizeType value = 0)
			: PriLib::ExplicitType<Config::MemorySizeType, 0>(value) {}

		MemorySize& operator+=(const MemorySize &other) {
			Config::MemorySizeType result;
			Config::MemorySizeType lhs = this->data;
			Config::MemorySizeType rhs = other.data;
			result = lhs + rhs;
			Config::CheckMemorySizeOverflow(result >= lhs && result >= rhs);
			this->data = result;
			return *this;
		}
		MemorySize& operator-=(const MemorySize &other) {
			Config::MemorySizeType result;
			Config::MemorySizeType lhs = this->data;
			Config::MemorySizeType rhs = other.data;
			result = lhs - rhs;
			Config::CheckMemorySizeOverflow(lhs >= rhs);
			this->data = result;
			return *this;
		}
		MemorySize& operator*=(const MemoryCount &other) {
			Config::MemorySizeType result;
			Config::MemorySizeType lhs = this->data;
			Config::MemoryCountType rhs = other.data;
			result = lhs * rhs;
			Config::CheckMemorySizeOverflow(rhs == 0 || result >= lhs);
			this->data = result;
			return *this;
		}
		MemorySize& operator/=(const MemoryCount &other) {
			Config::MemorySizeType result;
			Config::MemorySizeType lhs = this->data;
			Config::MemoryCountType rhs = other.data;
			result = lhs / rhs;
			Config::CheckMemorySizeOverflow(result <= lhs);
			this->data = result;
			return *this;
		}
		MemorySize operator+(const MemorySize &other) const {
			MemorySize result;
			return result += other;
		}
		MemorySize operator-(const MemorySize &other) const {
			MemorySize result;
			return result -= other;
		}
		MemorySize operator*(const MemoryCount &other) const {
			MemorySize result;
			return result *= other;
		}
		MemorySize operator/(const MemoryCount &other) const {
			MemorySize result;
			return result /= other;
		}

		int compare(const MemorySize &other) const {
			if (this->data == other.data)
				return 0;
			else if (this->data > other.data)
				return 1;
			else
				return -1;
		}
	};

	inline bool operator>(const MemorySize &lhs, const MemorySize &rhs) { return lhs.compare(rhs) > 0; }
	inline bool operator>=(const MemorySize &lhs, const MemorySize &rhs) { return lhs.compare(rhs) >= 0; }
	inline bool operator<(const MemorySize &lhs, const MemorySize &rhs) { return lhs.compare(rhs) < 0; }
	inline bool operator<=(const MemorySize &lhs, const MemorySize &rhs) { return lhs.compare(rhs) <= 0; }
	inline bool operator==(const MemorySize &lhs, const MemorySize &rhs) { return lhs.compare(rhs) == 0; }

	using TypeNameID = HashID;

	struct TypeInfo
	{
		TypeIndex index;
		MemorySize size;
		TypeNameID name;
	};

	class TypeInfoMap
	{
	public:
		explicit TypeInfoMap(HashStringPool &hashStringPool);

		bool insert(const HashID &nameid, const TypeInfo &info);
		bool find(const HashID &nameid, TypeIndex &id) const {
			auto iter = _keymap.find(nameid);
			if (iter != _keymap.end()) {
				id.data = iter->second;
				return true;
			}
			return false;
		}
		const TypeInfo& at(TypeIndex id) const {
			return _data.at(id.data);
		}
		const TypeInfo& at(const HashID &nameid) const {
			TypeIndex id;
			bool result = find(nameid, id);
			assert(result);
			return _data.at(id.data);
		}
		TypeInfo& at(const HashID &nameid) {
			return const_cast<TypeInfo&>(const_cast<const TypeInfoMap*>(this)->at(nameid));
		}

	private:
		std::map<HashID, Config::TypeIndexType> _keymap;
		std::vector<TypeInfo> _data;
	};
}
