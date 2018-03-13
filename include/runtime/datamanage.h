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


			enum DstRegisterMode
			{
				drm_null,
				drm_register_dynamic,
				drm_register_static,
			};
			struct DstData {
				DstRegisterMode mode = drm_null;
				DataPointer *datap = nullptr;
				TypeIndex *typep = nullptr;
			};
			struct SrcData {
				DataPointer data;
				TypeIndex type;
			};
			struct ResultData {
				DataRegisterType rtype = rt_null;
				DataRegister *drp = nullptr;
			};
			void MoveRegister(Environment &env, const DstData &dst, const SrcData &src);
			void LoadData(Environment &env, const DstData &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize);
			void LoadDataPointer(Environment &env, const DstData &dst, ConstDataPointer src, MemorySize srcsize);
			void Call(Environment &env, const Runtime::Function &func, const ResultData &dst, const PriLib::lightlist<SrcData> &arglist);
			
			DstData GetDstData(DataRegisterDynamic &dst);
			DstData GetDstData(DataRegisterStatic &dst);
			DstData GetDstDataZero();
			DstData GetDstDataResult(Environment &env);

			SrcData GetSrcData(const DataRegisterDynamic &src);
			SrcData GetSrcData(const DataRegisterStatic &src, TypeIndex type);

			void Debug_PrintRegister(Environment &env, const DataRegisterDynamic &src);
			void Debug_PrintRegister(Environment &env, const DataRegisterStatic &src, TypeIndex type);
		}
	}
}
