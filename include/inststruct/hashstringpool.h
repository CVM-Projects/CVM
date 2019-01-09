#pragma once
#include <functional>
#include <map>
#include <cassert>
#include "../prilib/include/explicittype.h"

namespace CVM
{
	DefineExplicitType(HashID, size_t);

	inline bool operator<(const HashID &lhs, const HashID &rhs) {
		return lhs.data < rhs.data;
	}

	class HashStringPool
	{
	public:
		class HashIDGetter {
		public:
			explicit HashIDGetter(const HashID &hashId, const HashStringPool &hashStringPool)
				: hashId(hashId), hashStringPool(hashStringPool) {}
			const std::string& operator()() {
				return hashStringPool.get(hashId);
			}

		private:
			const HashID &hashId;
			const HashStringPool &hashStringPool;
		};

	public:
		explicit HashStringPool() = default;

		const std::string& get(const HashID &id) const {
			return _data.at(id.data);
		}
		HashIDGetter getIDGetter(const HashID &id) const {
			return HashIDGetter(id, *this);
		}

		HashID getID(const std::string &value) const {
			return HashID(_getHash(value));
		}

		HashID insert(const std::string &value) {
			size_t id = getID(value).data;
			if (_data.find(id) == _data.end()) {
				_data.insert({ id, value });
			}
			else {
				assert(_data.at(id) == value);
			}
			return HashID(id);
		}

	private:
		std::hash<std::string> _getHash;
		std::map<size_t, std::string> _data;
	};
}
