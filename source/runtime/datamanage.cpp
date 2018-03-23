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
			DstData GetDstDataZero() {
				return DstData { drm_null };
			}
			DstData GetDstDataResult(Environment &env) {
				switch (env.get_result().rtype) {
				case rt_dynamic:
					return GetDstData(static_cast<DataRegisterDynamic&>(*env.get_result().drp));
				case rt_static:
					return GetDstData(static_cast<DataRegisterStatic&>(*env.get_result().drp));
				default:
					assert(false);
					return DstData();
				}
			}
			DstData GetDstData(const ResultData &rdata) {
				switch (rdata.rtype) {
				case rt_null:
					return GetDstDataZero();
				case rt_dynamic:
					return GetDstData(static_cast<DataRegisterDynamic&>(*rdata.drp));
				case rt_static:
					return GetDstData(static_cast<DataRegisterStatic&>(*rdata.drp));
				default:
					assert(false);
					return DstData();
				}
			}
			SrcData GetSrcData(const DataRegisterDynamic &src) {
				return SrcData { src.data, src.type };
			}
			SrcData GetSrcData(const DataRegisterStatic &src, TypeIndex type) {
				return SrcData { src.data, type };
			}
			DataPointer GetDataPointer(Environment &env, const DstData &dst) {
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

			static void CallInst(Environment &env, const Runtime::Function &func, const ResultData &dst, const PriLib::lightlist<SrcData> &arglist) {
				const Runtime::InstFunction &instf = static_cast<const Runtime::InstFunction &>(func);
				auto senv = Compile::CreateLoaclEnvironment(instf, env.getTypeInfoMap());
				auto argp = arglist.begin();
				for (Config::RegisterIndexType i = 0; i != instf.info().get_accesser().argument_count(); ++i) {
					const auto &arg = instf.info().arglist()[i];
					DstData dst;
					if (senv->is_dyvarb(arg, e_current)) {
						dst = GetDstData(senv->get_dyvarb(arg, e_current));
					}
					else if (senv->is_stvarb(arg, e_current)) {
						dst = GetDstData(senv->get_stvarb(arg, e_current));
					}
					else {
						assert(false);
					}
					MoveRegister(env, dst, *argp);
					++argp;
				}
				senv->get_result().rtype = dst.rtype;
				senv->get_result().drp = dst.drp;
				env.addSubEnvironment(senv);
				env.GEnv().getVM().Call(*senv);
				env.removeSubEnvironment(senv);
			}
			static void CallPtr(Environment &env, const Runtime::Function &func, const ResultData &dst, const PriLib::lightlist<SrcData> &arglist) {
				auto fp = static_cast<const Runtime::PointerFunction &>(func).data();
				PointerFunction::ArgumentList::creater aplist_creater(arglist.size());
				for (auto &arg : arglist) {
					aplist_creater.push_back(GetDataPointer(arg));
				}
				PointerFunction::ArgumentList aplist = aplist_creater.data();

				PointerFunction::Result xdst = GetDataPointer(env, GetDstData(dst));
				fp(xdst, aplist);
			}

			void Call(Environment &env, const Runtime::Function &func, const ResultData &dst, const PriLib::lightlist<SrcData> &arglist) {
				switch (func.type()) {
				case ft_null:
					break;
				case ft_inst:
					CallInst(env, func, dst, arglist);
					break;
				case ft_ptr:
					CallPtr(env, func, dst, arglist);
					break;
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
