#pragma once
#include "memory.h"
#include "register.h"
#include "environment.h"
#include "../lcmm/include/lcmm.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage
		{
			std::string ToStringData(Runtime::ConstDataPointer dp, MemorySize size);
			DataPointer Alloc(MemorySize size);
			DataPointer AllocClear(MemorySize size);

			// Move
			void MoveRegisterDdDd(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src);
			void MoveRegisterDsDd(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src);
			void MoveRegisterDdDs(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype);
			void MoveRegisterDsDs(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype);

			void MoveRegisterResDd(Environment &env, const DataRegisterDynamic &src);
			void MoveRegisterResDs(Environment &env, const DataRegisterStatic &src, TypeIndex srctype);
			void MoveRegisterDdRes(Environment &env, DataRegisterDynamic &dst, TypeIndex restype);
			void MoveRegisterDsRes(Environment &env, DataRegisterStatic &dst, TypeIndex restype);

			// Load
			void LoadDataDd(Environment &env, DataRegisterDynamic &dst, TypeIndex expecttype, ConstDataPointer src, MemorySize srcsize);
			void LoadDataDs(Environment &env, DataRegisterStatic &dst, TypeIndex dsttype, ConstDataPointer src, MemorySize srcsize);
			void LoadDataRes(Environment &env, TypeIndex restype, ConstDataPointer src, MemorySize srcsize);
			
			// LoadPointer
			void LoadDataPointerDd(Environment &env, DataRegisterDynamic &dst, ConstDataPointer src);
			void LoadDataPointerDs(Environment &env, DataRegisterStatic &dst, TypeIndex dsttype, ConstDataPointer src);
			void LoadDataPointerRes(Environment &env, TypeIndex restype, ConstDataPointer src);

			void CallDds(Environment &env, Config::RegisterIndexType dst, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist);
			void CallRes(Environment &env, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist);
			void CallZero(Environment &env, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist);

			// Debug
			void Debug_PrintRegister(Environment &env, const DataRegisterDynamic &src);
			void Debug_PrintRegister(Environment &env, const DataRegisterStatic &src, TypeIndex type);
		}
	}
}
