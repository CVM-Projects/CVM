#include "basic.h"
#include "typeinfo.h"
#include "runtime/datapointer.h"

namespace CVM
{
	static void InsertTypeInside(TypeInfoMap &tim);

	TypeInfoMap::TypeInfoMap() {
		InsertTypeInside(*this);
	}

	bool TypeInfoMap::insert(const std::string &name, const TypeInfo &info) {
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

	static void InsertTypeInside(TypeInfoMap &tim) {
		static TypeInfo tis[] {
			TypeInfo { TypeIndex(T_Void), MemorySize(0), TypeName("cms#void") },
			TypeInfo { TypeIndex(T_Int8), MemorySize(1), TypeName("cms#int8") },
			TypeInfo { TypeIndex(T_Int16), MemorySize(2), TypeName("cms#int16") },
			TypeInfo { TypeIndex(T_Int32), MemorySize(4), TypeName("cms#int32") },
			TypeInfo { TypeIndex(T_Int64), MemorySize(8), TypeName("cms#int64") },
			TypeInfo { TypeIndex(T_UInt8), MemorySize(1), TypeName("cms#uint8") },
			TypeInfo { TypeIndex(T_UInt16), MemorySize(2), TypeName("cms#uint16") },
			TypeInfo { TypeIndex(T_UInt32), MemorySize(4), TypeName("cms#uint32") },
			TypeInfo { TypeIndex(T_UInt64), MemorySize(8), TypeName("cms#uint64") },
			TypeInfo { TypeIndex(T_Pointer), Runtime::DataPointer::Size, TypeName("cms#pointer") },
			TypeInfo { TypeIndex(T_Environment), Runtime::DataPointer::Size, TypeName("cms#environment") },
			TypeInfo { TypeIndex(T_VirtualMachine), Runtime::DataPointer::Size, TypeName("cms#virtualmachine") },
		};

		for (auto &ti : tis) {
			tim.insert(ti.name.data, ti);
		}
	}
}
