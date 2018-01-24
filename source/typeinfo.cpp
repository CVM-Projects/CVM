#include "basic.h"
#include "typeinfo.h"

namespace CVM
{
	bool TypeInfoMap::insert(const std::string & name, const TypeInfo & info) {
		TypeInfo ninfo(info);
		assert(_data.size() < UINT32_MAX);
		ninfo.index.data = static_cast<uint32_t>(_data.size());
		ninfo.name.data = name;
		// Insert
		if (_keymap.find(name) != _keymap.end()) {
			return false;
		}
		else {
			_keymap.insert({ name, ninfo.index.data });
			_data.push_back(ninfo);
			return true;
		}
	}
}
