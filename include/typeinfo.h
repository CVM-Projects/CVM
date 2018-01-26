#pragma once
#include "../prilib/include/explicittype.h"
#include <cstdint>
#include <map>

namespace CVM
{
	using TypeIndex = PriLib::ExplicitType<uint32_t, 0>;
	struct TypeIndexLess {
		bool operator()(const TypeIndex &t1, const TypeIndex &t2) const {
			return t1.data < t2.data;
		}
	};

	using MemorySize = PriLib::ExplicitType<uint32_t, 0>;
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
		std::map<std::string, uint32_t> _keymap;
		std::vector<TypeInfo> _data;
	};
}
