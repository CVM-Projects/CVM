#pragma once
#include "basic.h"
#include "function.h"

namespace CVM
{
	namespace InstStruct
	{
		struct IdentKeyTable
		{
			using FuncPtr = std::shared_ptr<InstStruct::Function>;

			bool hasKey(const std::string &key) {
				return keytable.find(key) != keytable.end();
			}
			bool insert(const std::string &key) {
				if (!hasKey(key)) {
					_insert(key);
					return true;
				}
				return false;
			}
			Config::FuncIndexType getID(const std::string &key) {
				auto iter = keytable.find(key);
				if (iter == keytable.end()) {
					return _insert(key);
				}
				return iter->second;
			}
			auto& getData(Config::FuncIndexType id) {
				return functable.at(id);
			}
			auto& getData(const std::string &key) {
				return getData(getID(key));
			}
			/*auto begin() const {
				return functable.begin();
			}
			auto end() const {
				return functable.end();
			}*/

			template <typename _FTy>
			void each(_FTy f) {
				for (auto pair : keytable) {
					f(pair.second, functable.at(pair.second));
				}
			}

		private:
			std::map<std::string, Config::FuncIndexType> keytable;
			std::vector<FuncPtr> functable;

			Config::FuncIndexType _insert(const std::string &key) {
				assert(keytable.size() < std::numeric_limits<Config::FuncIndexType>::max());
				Config::FuncIndexType id = static_cast<Config::FuncIndexType>(keytable.size());
				keytable.insert({ key, id });
				while (functable.size() <= id)
					functable.push_back(nullptr);
				return id;
			}
		};
	}
}
