#pragma once
#include "../prilib/include/explicittype.h"
#include <cstdint>
#include <map>
#include "config.h"

namespace CVM
{
	using TypeIndex = PriLib::ExplicitType<Config::TypeIndexType, 0>;
	struct TypeIndexLess {
		bool operator()(const TypeIndex &t1, const TypeIndex &t2) const {
			return t1.data < t2.data;
		}
	};

	using MemoryCount = PriLib::ExplicitType<Config::MemoryCountType, 0>;

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
	};

	using TypeName = PriLib::ExplicitType<std::string>;

	struct TypeInfo
	{
		TypeIndex index;
		MemorySize size;
		TypeName name;
	};

	class TypeInfoMap
	{
	public:
		explicit TypeInfoMap() {
			insert("nil", TypeInfo());
		}

		bool insert(const std::string &name, const TypeInfo &info);
		bool find(const std::string &name, TypeIndex &id) const {
			auto iter = _keymap.find(name);
			if (iter != _keymap.end()) {
				id.data = iter->second;
				return true;
			}
			return false;
		}
		const TypeInfo& at(TypeIndex id) const {
			return _data.at(id.data);
		}
		const TypeInfo& at(const std::string &name) const {
			TypeIndex id;
			bool result = find(name, id);
			assert(result);
			return _data.at(id.data);
		}
		TypeInfo& at(const std::string &name) {
			return const_cast<TypeInfo&>(const_cast<const TypeInfoMap*>(this)->at(name));
		}

	private:
		std::map<std::string, Config::TypeIndexType> _keymap;
		std::vector<TypeInfo> _data;
	};
}
