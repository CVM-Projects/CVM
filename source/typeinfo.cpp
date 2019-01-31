#include "basic.h"
#include "typeinfo.h"
#include "runtime/datapointer.h"
#include "inststruct/hashstringpool.h"

namespace CVM
{
	static void InsertTypeInside(TypeInfoMap &tim, HashStringPool &hashStringPool);

	TypeInfoMap::TypeInfoMap(HashStringPool &hashStringPool) {
		InsertTypeInside(*this, hashStringPool);
	}

	bool TypeInfoMap::insert(const HashID &nameid, const TypeInfo &info) {
		TypeInfo ninfo(info);
		assert(_data.size() < std::numeric_limits<Config::TypeIndexType>::max());
		ninfo.index.data = static_cast<Config::TypeIndexType>(_data.size());
		ninfo.name = nameid;
		// Insert
		if (_keymap.find(nameid) != _keymap.end()) {
			return false;
		}
		else {
			_keymap.insert({ nameid, ninfo.index.data });
			_data.push_back(ninfo);
			return true;
		}
	}

	static void InsertTypeInside(TypeInfoMap &tim, HashStringPool &hashStringPool) {
		static TypeInfo tis[] {
			TypeInfo { TypeIndex(T_Void), MemorySize(0), TypeNameID(hashStringPool.insert("cms#void")) },
			TypeInfo { TypeIndex(T_Int8), MemorySize(1), TypeNameID(hashStringPool.insert("cms#int8")) },
			TypeInfo { TypeIndex(T_Int16), MemorySize(2), TypeNameID(hashStringPool.insert("cms#int16")) },
			TypeInfo { TypeIndex(T_Int32), MemorySize(4), TypeNameID(hashStringPool.insert("cms#int32")) },
			TypeInfo { TypeIndex(T_Int64), MemorySize(8), TypeNameID(hashStringPool.insert("cms#int64")) },
			TypeInfo { TypeIndex(T_UInt8), MemorySize(1), TypeNameID(hashStringPool.insert("cms#uint8")) },
			TypeInfo { TypeIndex(T_UInt16), MemorySize(2), TypeNameID(hashStringPool.insert("cms#uint16")) },
			TypeInfo { TypeIndex(T_UInt32), MemorySize(4), TypeNameID(hashStringPool.insert("cms#uint32")) },
			TypeInfo { TypeIndex(T_UInt64), MemorySize(8), TypeNameID(hashStringPool.insert("cms#uint64")) },
			TypeInfo { TypeIndex(T_Pointer), Runtime::DataPointer::Size, TypeNameID(hashStringPool.insert("cms#pointer")) },
			TypeInfo { TypeIndex(T_Environment), Runtime::DataPointer::Size, TypeNameID(hashStringPool.insert("cms#environment")) },
			TypeInfo { TypeIndex(T_VirtualMachine), Runtime::DataPointer::Size, TypeNameID(hashStringPool.insert("cms#virtualmachine")) },
		};

		for (auto &ti : tis) {
			tim.insert(ti.name, ti);
		}
	}
}
