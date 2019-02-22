#pragma once
#include "basic.h"
#include "function.h"
#include "hashstringpool.h"
#include <mutex>

namespace CVM
{
	namespace InstStruct
	{
		struct IdentKeyTable
		{
			using FuncPtr = std::shared_ptr<InstStruct::Function>;

			bool hasKey(const HashID &keyid) const {
				return keytable.find(keyid) != keytable.end();
			}
			bool insert(const HashID &keyid) {
				if (!hasKey(keyid)) {
					_insert(keyid);
					return true;
				}
				return false;
			}
			Config::FuncIndexType getID(const HashID &keyid) {
				auto iter = keytable.find(keyid);
				if (iter == keytable.end()) {
					return _insert(keyid);
				}
				return iter->second;
			}
			auto& getData(Config::FuncIndexType id) {
				return functable.at(id);
			}
			auto& getData(const HashID &keyid) {
				return getData(getID(keyid));
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
			std::map<HashID, Config::FuncIndexType> keytable;
			std::vector<FuncPtr> functable;
			std::mutex _mutex;

			Config::FuncIndexType _insert(const HashID &keyid) {
				std::lock_guard<std::mutex> lock(_mutex);
				assert(keytable.size() < std::numeric_limits<Config::FuncIndexType>::max());
				Config::FuncIndexType id = static_cast<Config::FuncIndexType>(keytable.size());
				keytable.insert({ keyid, id });
				while (functable.size() <= id)
					functable.push_back(nullptr);
				return id;
			}
		};
	}
}
