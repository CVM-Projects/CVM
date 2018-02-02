#pragma once
#include "memory.h"
#include "register.h"
#include "environment.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage
		{
			std::string ToStringData(Runtime::DataPointer dp, MemorySize size);
			DataPointer Alloc(MemorySize size);
			DataPointer AllocClear(MemorySize size);

			void MoveRegisterDD(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src);
			void MoveRegisterSD(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src);
			void MoveRegisterDS(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype);
			void MoveRegisterSS(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype);

			void LoadDataD(Environment &env, DataRegisterDynamic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize);
			void LoadDataS(Environment &env, DataRegisterStatic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize);

			void Debug_PrintRegisterD(Environment &env, const DataRegisterDynamic &src);
			void Debug_PrintRegisterS(Environment &env, const DataRegisterStatic &src, TypeIndex type);
		}
	}
}
