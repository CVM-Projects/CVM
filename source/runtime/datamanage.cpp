#include "runtime/datamanage.h"
#include "charptr.h"
#include "range.h"
#include "prints.h"

namespace CVM
{
	namespace Runtime
	{
		namespace DataManage
		{
			static void CopyTo(DataPointer dst, DataPointer src, MemorySize size) {
				Common::Memory::copyTo(dst.get(), src.get(), size.data);
			}
			static MemorySize GetSize(Environment &env, const TypeIndex &type) {
				return env.getType(type).size;
			}

			DataPointer Alloc(MemorySize size) {
				return DataPointer(malloc(size.data));
			}

			static std::string ToStringData(byte *bp, size_t size) {
				Common::charptr result(7 + 3 * size);

				byte *begin = bp;
				byte *end = bp + size;

				result.set(0, "[data:");

				char *rp = (char*)result + 6;

				for (auto ptr : Common::range(begin, end)) {
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

			void LoadData(Environment &env, DataRegisterDynamic &dst, DataPointer src, TypeIndex srctype) {
				dst.data = Alloc(GetSize(env, srctype));
				CopyTo(dst.data, src, GetSize(env, srctype));
				dst.type = srctype;
			}
			void LoadData(Environment &env, DataRegisterStatic &dst, DataPointer src, TypeIndex srctype) {
				CopyTo(dst.data, src, GetSize(env, srctype));
			}

			void Debug_PrintRegister(Environment &env, const DataRegisterDynamic &src) {
				Common::Output::println(ToStringData(src.data, GetSize(env, src.type)));
			}
			void Debug_PrintRegister(Environment &env, const DataRegisterStatic &src, TypeIndex type) {
				Common::Output::println(ToStringData(src.data, GetSize(env, type)));
			}
		}
	}
}
