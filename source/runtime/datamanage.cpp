#include "basic.h"
#include "runtime/datamanage.h"
#include "compile.h"

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

			DstData GetDstData(DataRegisterDynamic &dst) {
				return DstData { drm_register_dynamic, &dst.data, &dst.type };
			}
			DstData GetDstData(DataRegisterStatic &dst) {
				return DstData { drm_register_static, &dst.data, nullptr };
			}
			SrcData GetSrcData(const DataRegisterDynamic &src) {
				return SrcData { src.data, src.type };
			}
			SrcData GetSrcData(const DataRegisterStatic &src, TypeIndex type) {
				return SrcData { src.data, type };
			}
			DataPointer GetDataPointer(const DstData &dst) {
				switch (dst.mode) {
				case drm_null:
					return DataPointer(nullptr);
				case drm_register_dynamic:
					return *dst.datap;
				case drm_register_static:
					return *dst.datap;
				default:
					assert(false);
					return DataPointer(nullptr);
				}
			}
			DataPointer GetDataPointer(const SrcData &src) {
				return src.data;
			}

			void MoveRegister(Environment &env, const DstData &dst, const SrcData &src) {
				switch (dst.mode) {
				case drm_null:
					break;
				case drm_register_dynamic:
					*dst.datap = src.data;
					break;
				case drm_register_static:
					CopyTo(*dst.datap, src.data, GetSize(env, src.type));
					break;
				default:
					assert(false);
				}
				if (dst.typep) {
					*dst.typep = src.type;
				}
			}

			void LoadData(Environment &env, const DstData &dst, ConstDataPointer src, TypeIndex dsttype, MemorySize srcsize) {
				switch (dst.mode) {
				case drm_null:
					break;
				case drm_register_dynamic:
					*dst.datap = AllocClear(GetSize(env, dsttype));
					CopyTo(*dst.datap, src, MemorySize(std::min(GetSize(env, dsttype).data, srcsize.data)));
					*dst.typep = dsttype;
					break;
				case drm_register_static:
					Clear(*dst.datap, GetSize(env, dsttype));
					CopyTo(*dst.datap, src, MemorySize(std::min(GetSize(env, dsttype).data, srcsize.data)));
					break;
				default:
					assert(false);
				}
			}

			void LoadDataPointer(Environment &env, const DstData &dst, ConstDataPointer src, MemorySize srcsize) {
				switch (dst.mode) {
				case drm_null:
					break;
				case drm_register_dynamic:
				{
					DataPointer buffer = AllocClear(srcsize);
					void *address = buffer.get();
					CopyTo(buffer, src, srcsize);
					*dst.datap = AllocClear(DataPointer::Size);
					CopyTo(*dst.datap, DataPointer(&address), DataPointer::Size);
					*dst.typep = TypeIndex(T_Pointer);
					break;
				}
				case drm_register_static:
				{
					DataPointer buffer = AllocClear(srcsize);
					void *address = buffer.get();
					CopyTo(buffer, src, srcsize);
					CopyTo(*dst.datap, DataPointer(&address), DataPointer::Size);
					break;
				}
				default:
					assert(false);
				}
			}

			void Call(Environment &env, const Runtime::Function &func, const DstData &dst, const PriLib::lightlist<SrcData> &arglist) {
				switch (func.type()) {
				case ft_null:
					break;
				case ft_inst:
				{
					auto instf = static_cast<const Runtime::InstFunction &>(func);
					auto senv = Compile::CreateLoaclEnvironment(instf, env.GetTypeInfoMap());
					env.addSubEnvironment(senv);
					env.GEnv().getVM().Call(*senv);
					break;
				}
				case ft_ptr:
				{
					auto fp = static_cast<const Runtime::PointerFunction &>(func).data();
					PointerFunction::ArgumentList::creater aplist_creater(arglist.size());
					for (auto &arg : arglist) {
						aplist_creater.push_back(GetDataPointer(arg));
					}
					PointerFunction::ArgumentList aplist = aplist_creater.data();
					PointerFunction::Result xdst = GetDataPointer(dst);
					fp(xdst, aplist);
					break;
				}
				}
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
