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
			std::string ToStringData(Runtime::ConstDataPointer dp, MemorySize size);
			DataPointer Alloc(MemorySize size);
			DataPointer AllocClear(MemorySize size);


			enum MoveRegisterMode
			{
				mr_copy_ptr,
				mr_copy_memory,
			};
			struct DstData {
				MoveRegisterMode mode;
				DataPointer *datap = nullptr;
				TypeIndex *typep = nullptr;
			};
			struct SrcData {
				DataPointer data;
				TypeIndex type;
			};
			void MoveRegister(Environment &env, const DstData &dst, const SrcData &src);

			DstData GetDstDataD(DataRegisterDynamic &dst);
			DstData GetDstDataS(DataRegisterStatic &dst);
			SrcData GetSrcDataD(const DataRegisterDynamic &src);
			SrcData GetSrcDataS(const DataRegisterStatic &src, TypeIndex type);

			void LoadDataD(Environment &env, DataRegisterDynamic &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize);
			void LoadDataS(Environment &env, DataRegisterStatic &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize);

			void Debug_PrintRegisterD(Environment &env, const DataRegisterDynamic &src);
			void Debug_PrintRegisterS(Environment &env, const DataRegisterStatic &src, TypeIndex type);
		}
	}
}
