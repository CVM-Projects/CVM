#pragma once
#include "function.h"
#include "../../prilib/include/bijectionmap.h"
#include "inststruct/hashstringpool.h"

namespace CVM
{
	namespace Runtime
	{
		class FuncTable : public std::map<Config::FuncIndexType, Function*>
		{
		public:
			FuncTable() = default;
			FuncTable(const FuncTable &) = delete;

			~FuncTable() {
				for (auto &pair : *this) {
					delete pair.second;
				}
			}
		};
		using PtrFuncMap = std::map<HashID, Runtime::PointerFunction::Func*>;
	}
}
