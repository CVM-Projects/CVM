#include "basic.h"
#include "runtime/datamanage.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage
		{
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

			void MoveRegisterDD(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src) {
				dst = src;
			}
			void MoveRegisterSD(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src) {
				CopyTo(dst.data, src.data, GetSize(env, src.type));
			}
			void MoveRegisterDS(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				dst.data = src.data;
				dst.type = srctype;
			}
			void MoveRegisterSS(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				CopyTo(dst.data, src.data, GetSize(env, srctype));
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
