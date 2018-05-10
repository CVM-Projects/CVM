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
			void Free(DataPointer dp) {
				std::free(dp.get());
			}

			static std::string ToStringData(const byte *bp, size_t size) {
				return "[data: " + PriLib::Convert::to_hex(bp, size) + "]";
			}

			std::string ToStringData(CVM::Runtime::ConstDataPointer dp, CVM::MemorySize size) {
				return ToStringData(dp.get<byte>(), size.data);
			}

			//

			void Alloc(LCMM::MemoryManager &mm, DataRegisterDynamic &drd, MemorySize size) {
				mm.Alloc(drd);
				drd.data = Alloc(size);
			}


			//


			void MoveRegisterDdDd(Environment &env, DataRegisterDynamic &dst, const DataRegisterDynamic &src) {
				dst = src;
			}
			void MoveRegisterDsDd(Environment &env, DataRegisterStatic &dst, const DataRegisterDynamic &src) {
				CopyTo(dst.data, src.data, GetSize(env, src.type));
			}
			void MoveRegisterDdDs(Environment &env, DataRegisterDynamic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				dst.data = src.data;
				dst.type = srctype;
			}
			void MoveRegisterDsDs(Environment &env, DataRegisterStatic &dst, const DataRegisterStatic &src, TypeIndex srctype) {
				CopyTo(dst.data, src.data, GetSize(env, srctype));
			}

			template <typename FTy1, typename FTy2>
			static void DoResultRegister(Environment &env, FTy1 func_dynamic, FTy2 func_static) {
				ResultRegister &res = env.get_result();
				switch (res.rtype) {
				case rt_null: assert(false); break;
				case rt_dynamic: func_dynamic(static_cast<DataRegisterDynamic&>(*res.drp)); break;
				case rt_static: func_static(static_cast<DataRegisterStatic&>(*res.drp)); break;
				default: assert(false);
				}
			}

			void MoveRegisterResDd(Environment &env, const DataRegisterDynamic &src) {
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { MoveRegisterDdDd(env, res, src); },
					[&](DataRegisterStatic &res) { MoveRegisterDsDd(env, res, src); });
			}
			void MoveRegisterResDs(Environment &env, const DataRegisterStatic &src, TypeIndex srctype) {
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { MoveRegisterDdDs(env, res, src, srctype); },
					[&](DataRegisterStatic &res) { MoveRegisterDsDs(env, res, src, srctype); });
			}
			void MoveRegisterDdRes(Environment &env, DataRegisterDynamic &dst, TypeIndex restype) {
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { MoveRegisterDdDd(env, dst, res); },
					[&](DataRegisterStatic &res) { MoveRegisterDdDs(env, dst, res, restype); });
			}
			void MoveRegisterDsRes(Environment &env, DataRegisterStatic &dst, TypeIndex restype) {
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { MoveRegisterDsDd(env, dst, res); },
					[&](DataRegisterStatic &res) { MoveRegisterDsDs(env, dst, res, restype); });
			}


			void LoadDataDd(Environment &env, DataRegisterDynamic &dst, TypeIndex expecttype, ConstDataPointer src, MemorySize srcsize) {
				//Free(dst.data); // TODO : Use LCMM
				dst.data = AllocClear(GetSize(env, expecttype));  // TODO
				CopyTo(dst.data, src, MemorySize(std::min(GetSize(env, expecttype).data, srcsize.data)));
				dst.type = expecttype;
				// TODO!
			}
			void LoadDataDs(Environment &env, DataRegisterStatic &dst, TypeIndex dsttype, ConstDataPointer src, MemorySize srcsize) {
				Clear(dst.data, GetSize(env, dsttype));
				CopyTo(dst.data, src, MemorySize(std::min(GetSize(env, dsttype).data, srcsize.data)));
			}
			void LoadDataRes(Environment &env, TypeIndex restype, ConstDataPointer src, MemorySize srcsize) {
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { LoadDataDd(env, res, restype, src, srcsize); },
					[&](DataRegisterStatic &res) { LoadDataDs(env, res, restype, src, srcsize); });
			}
			void LoadDataPointerDd(Environment &env, DataRegisterDynamic &dst, ConstDataPointer src) {
				// Only copy pointer
				//Free(dst.data); // TODO : Use LCMM
				dst.data = AllocClear(DataPointer::Size); // TODO
				auto p = src.get();
				CopyTo(dst.data, ConstDataPointer(&p), DataPointer::Size);
				// TODO : Sign with const data!!!
			}
			void LoadDataPointerDs(Environment &env, DataRegisterStatic &dst, TypeIndex dsttype, ConstDataPointer src) {
				// Only copy pointer
				assert(GetSize(env, dsttype) >= DataPointer::Size);
				Clear(dst.data, GetSize(env, dsttype));
				auto p = src.get();
				CopyTo(dst.data, ConstDataPointer(&p), DataPointer::Size);
				// TODO : Sign with const data!!!
			}
			void LoadDataPointerRes(Environment &env, TypeIndex restype, ConstDataPointer src) {
				// Only copy pointer
				DoResultRegister(env,
					[&](DataRegisterDynamic &res) { LoadDataPointerDd(env, res, src); },
					[&](DataRegisterStatic &res) { LoadDataPointerDs(env, res, restype, src); });
			}

			void Debug_PrintRegisterD(Environment &env, const DataRegisterDynamic &src) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, src.type)));
			}
			void Debug_PrintRegisterS(Environment &env, const DataRegisterStatic &src, TypeIndex type) {
				PriLib::Output::println(ToStringData(src.data, GetSize(env, type)));
			}

			//

			DstData GetDstData(DataRegisterDynamic &dst) {
				return DstData{ drm_register_dynamic, &dst.data, &dst.type };
			}
			DstData GetDstData(DataRegisterStatic &dst) {
				return DstData{ drm_register_static, &dst.data, nullptr };
			}
			DstData GetDstDataZero() {
				return DstData{ drm_null };
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
				return SrcData{ src.data, src.type };
			}
			SrcData GetSrcData(const DataRegisterStatic &src, TypeIndex type) {
				return SrcData{ src.data, type };
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


			// TODO : ResultData dst -> function operation
			void CallBase(Environment &env, const ResultData &dst, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist) {
				auto &table = env.GEnv().getFuncTable();
				auto iter = table.find(fid);

				if (iter != table.end()) {
					const Runtime::Function &func = *iter->second;
					PriLib::lightlist<SrcData>::creater arglist_creator(arglist.size());

					for (auto rid : arglist) {
						if (env.is_dyvarb(rid)) {
							arglist_creator.push_back(GetSrcData(env.get_dyvarb(rid)));
						}
						else if (env.is_stvarb(rid)) {
							assert(env.isLocal());
							arglist_creator.push_back(GetSrcData(env.get_stvarb(rid),
								((Runtime::LocalEnvironment&)env)._func.info().get_stvarb_type(rid)));
						}
					}
					// TODO
					Call(env, func, dst, arglist_creator.data());
				}
				else {
					println("Unfind function (id = ", fid, ").");
					assert(false);
				}
			}

			void CallDds(Environment &env, Config::RegisterIndexType dst, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist) {
				Runtime::DataManage::ResultData res;
				if (env.is_dyvarb(dst))
					res = Runtime::DataManage::ResultData{ Runtime::rt_dynamic, &env.get_dyvarb(dst) };
				else if (env.is_stvarb(dst))
					res = Runtime::DataManage::ResultData{ Runtime::rt_static, &env.get_stvarb(dst) };
				else
					assert(false);
				CallBase(env, res, fid, arglist);
			}
			void CallRes(Environment &env, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist) {
				Runtime::DataManage::ResultData res{ env.get_result().rtype, env.get_result().drp };
				CallBase(env, res, fid, arglist);
			}
			void CallZero(Environment &env, Config::FuncIndexType fid, const PriLib::lightlist<Config::RegisterIndexType> &arglist) {
				Runtime::DataManage::ResultData res{ Runtime::rt_null, nullptr };
				CallBase(env, res, fid, arglist);
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
