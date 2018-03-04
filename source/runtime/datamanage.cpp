#include "basic.h"
#include "runtime/datamanage.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage {
			static void CopyTo(DataPointer dst, ConstDataPointer src, MemorySize size) {
				PriLib::Memory::copyTo(dst.get(), src.get(), size.data);
			}

			static MemorySize GetSize(Environment &env, const TypeIndex &type) {
				return env.getType(type).size;
			}

			static void Clear(DataPointer dst, MemorySize size) {
				PriLib::Memory::clear(dst.get(), size.data);
			}

			DataPointer Alloc(MemorySize size) {
				return DataPointer(malloc(size.data));
			}

			DataPointer AllocClear(MemorySize size) {
				return DataPointer(calloc(size.data, 1));
			}

			static std::string ToStringData(const byte *bp, size_t size) {
				return "[data: " + PriLib::Convert::to_hex(bp, size) + "]";
			}

			std::string ToStringData(CVM::Runtime::ConstDataPointer dp, CVM::MemorySize size) {
				return ToStringData(dp.get<byte>(), size.data);
			}

			void MoveRegister(Environment &env, const DstData &dst, const SrcData &src) {
				switch (dst.mode) {
				case mr_copy_ptr:
					*dst.datap = src.data;
					break;
				case mr_copy_memory:
					CopyTo(*dst.datap, src.data, GetSize(env, src.type));
					break;
				}
				if (dst.typep) {
					*dst.typep = src.type;
				}
			}
			DstData GetDstDataD(DataRegisterDynamic &dst) {
				return DstData { mr_copy_ptr, &dst.data, &dst.type };
			}
			DstData GetDstDataS(DataRegisterStatic &dst) {
				return DstData { mr_copy_memory, &dst.data, nullptr };
			}
			SrcData GetSrcDataD(const DataRegisterDynamic &src) {
				return SrcData {src.data, src.type};
			}
			SrcData GetSrcDataS(const DataRegisterStatic &src, TypeIndex type) {
				return SrcData {src.data, type};
			}

			void LoadDataD(Environment &env, DataRegisterDynamic &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize) {
				dst.data = AllocClear(GetSize(env, dsttype));
				CopyTo(dst.data, src, MemorySize(std::min(GetSize(env, dsttype).data, srcsize.data)));
				dst.type = dsttype;
			}
			void LoadDataS(Environment &env, DataRegisterStatic &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize) {
				Clear(dst.data, GetSize(env, dsttype));
				CopyTo(dst.data, src, MemorySize(std::min(GetSize(env, dsttype).data, srcsize.data)));
			}

			void Debug_PrintRegisterD(Environment &env, const DataRegisterDynamic &src) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, src.type)));
			}
			void Debug_PrintRegisterS(Environment &env, const DataRegisterStatic &src, TypeIndex type) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, type)));
			}
		}
	}
}
