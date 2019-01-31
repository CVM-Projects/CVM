#pragma once
#include "hashstringpool.h"

namespace CVM
{
	DefineExplicitTypeWithValue(FileID, Config::FileIndexType, 0);
	inline bool operator<(const FileID &lhs, const FileID &rhs) {
		return lhs.data < rhs.data;
	}

	using FileNameMap = PriLib::BijectionKVMap<FileID, HashID>;
}
