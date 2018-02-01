#include "basic.h"
#include "runtime/datamanage.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage
		{
			static void CopyTo(DataPointer dst, DataPointer src, MemorySize size) {
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

			static std::string ToStringData(byte *bp, size_t size) {
				PriLib::charptr result(7 + 3 * size);

				byte *begin = bp;
				byte *end = bp + size;

				result.set(0, "[data:");

				char *rp = (char*)result + 6;

				for (auto ptr : PriLib::range(begin, end)) {
					int v;
					rp[0] = ' ';
					v = *ptr / 0x10;
					rp[1] = (v < 10) ? ('0' + v) : ('A' + v - 10);
					v = *ptr % 0x10;
					rp[2] = (v < 10) ? ('0' + v) : ('A' + v - 10);
					rp += 3;
				}

				rp[0] = ']';
				rp[1] = '\0';

				return result.to_string();
			}

			std::string ToStringData(CVM::Runtime::DataPointer dp, CVM::MemorySize size) {
				return ToStringData(dp.get<byte>(), size.data);
			}

			void MoveRegister(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src) {
				dst = src;
			}
			void MoveRegister(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src) {
				CopyTo(dst.data, src.data, GetSize(env, src.type));
			}
			void MoveRegister(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				dst.data = src.data;
				dst.type = srctype;
			}
			void MoveRegister(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				CopyTo(dst.data, src.data, GetSize(env, srctype));
			}

			void LoadData(Environment &env, DataRegisterDynamic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize) {
				dst.data = AllocClear(GetSize(env, dsttype));
				CopyTo(dst.data, src, srcsize);
				dst.type = dsttype;
			}
			void LoadData(Environment &env, DataRegisterStatic &dst, DataPointer src, TypeIndex dsttype, MemorySize srcsize) {
				Clear(dst.data, GetSize(env, dsttype));
				CopyTo(dst.data, src, srcsize);
			}

			void Debug_PrintRegister(Environment &env, const DataRegisterDynamic &src) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, src.type)));
			}
			void Debug_PrintRegister(Environment &env, const DataRegisterStatic &src, TypeIndex type) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, type)));
			}
		}
	}
}
