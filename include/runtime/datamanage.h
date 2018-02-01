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

			void MoveRegister(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src);
			void MoveRegister(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src);
			void MoveRegister(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype);
			void MoveRegister(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype);

			void LoadData(Environment &env, DataRegisterDynamic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize);
			void LoadData(Environment &env, DataRegisterStatic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize);

			void Debug_PrintRegister(Environment &env, const DataRegisterDynamic &src);
			void Debug_PrintRegister(Environment &env, const DataRegisterStatic &src, TypeIndex type);
		}
	}
}
