#include "basic.h"
#include "compile.h"
#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include "runtime/environment.h"
#include "runtime/datamanage.h"
#include "datapool.h"

namespace CVM
{
	namespace Compile
	{
		inline static Runtime::EnvType Convert(InstStruct::EnvType etype) {
			return static_cast<Runtime::EnvType>(static_cast<int>(etype));
		}

		inline static void CheckLocalEnv(Runtime::Environment &env) {
			if (!env.isLocal()) {
				puts("Error Occur because of not LocalEnvironment.");
				exit(-1);
			}
		}

		std::function<Runtime::DataRegisterDynamic&(Runtime::Environment &env)> compile_drd(const InstStruct::Register &r) {
			auto id = r.index();
			auto e = Convert(r.etype());
			return [=](Runtime::Environment &env) -> Runtime::DataRegisterDynamic& {
				return env.get_dyvarb(id, e);
			};
		}
		std::function<Runtime::DataRegisterStatic&(Runtime::Environment &env)> compile_drs(const InstStruct::Register &r) {
			auto id = r.index();
			auto e = Convert(r.etype());
			return [=](Runtime::Environment &env) -> Runtime::DataRegisterStatic& {
				return env.get_stvarb(id, e);
			};
		}
		Runtime::DataManage::DstData GetDstData(Runtime::Environment &env, Config::RegisterIndexType index, Runtime::EnvType envtype) {
			if (env.is_dyvarb(index, envtype)) {
				return Runtime::DataManage::GetDstDataD(env.get_dyvarb(index, envtype));
			}
			else if (env.is_stvarb(index, envtype)) {
				return Runtime::DataManage::GetDstDataS(env.get_stvarb(index, envtype));
			}
			assert(false);
			exit(1);
		}
		Runtime::DataManage::SrcData GetSrcData(Runtime::Environment &env, Config::RegisterIndexType index, Runtime::EnvType envtype, TypeIndex srctype) {
			if (env.is_dyvarb(index, envtype)) {
				return Runtime::DataManage::GetSrcDataD(env.get_dyvarb(index, envtype));
			}
			else if (env.is_stvarb(index, envtype)) {
				return Runtime::DataManage::GetSrcDataS(env.get_stvarb(index, envtype), srctype);
			}
			assert(false);
			exit(1);
		}


		Runtime::Instruction Compile(const InstStruct::Instruction &inst, const InstStruct::Function &func) {
			if (inst.instcode == InstStruct::i_mov) {
				auto &Inst = static_cast<const InstStruct::Insts::Move&>(inst);

				auto dst_e = Convert(Inst.dst.etype());
				auto src_e = Convert(Inst.src.etype());
				auto dst_id = Inst.dst.index();
				auto src_id = Inst.src.index();

				TypeIndex src_tid = Inst.src.have_tindex() ? Inst.src.tindex() : TypeIndex(0);

				using namespace Runtime::DataManage;
				DstData dst;
				SrcData src;

				std::function<DstData(Runtime::Environment &)> dst_f;
				std::function<SrcData(Runtime::Environment &)> src_f;


				if (Inst.dst.isPrivateDataRegister()) {
					if (func.is_dyvarb(dst_id)) {
						auto dst_df = compile_drd(Inst.dst);
						dst_f = [=](Runtime::Environment &env) { return GetDstDataD(dst_df(env)); };
					}
					else if (func.is_stvarb(dst_id)) {
						TypeIndex type = func.get_stvarb_type(dst_id);
						auto dst_sf = compile_drs(Inst.dst);
						dst_f = [=](Runtime::Environment &env) { return GetDstDataS(dst_sf(env)); };
					}
				}
				else {
					dst_f = [=](Runtime::Environment &env) { return GetDstData(env, dst_id, dst_e); };
				}

				if (Inst.src.isPrivateDataRegister()) {
					if (func.is_dyvarb(src_id)) {
						auto src_df = compile_drd(Inst.src);
						src_f = [=](Runtime::Environment &env) { return GetSrcDataD(src_df(env)); };
					}
					else if (func.is_stvarb(src_id)) {
						TypeIndex type = func.get_stvarb_type(src_id);
						auto src_sf = compile_drs(Inst.src);
						src_f = [=](Runtime::Environment &env) { return GetSrcDataS(src_sf(env), type); };
					}
				}
				else {
					src_f = [=](Runtime::Environment &env) { return GetSrcData(env, src_id, src_e, src_tid); };
				}

				return [=](Runtime::Environment &env) {
					MoveRegister(env, dst_f(env), src_f(env));
				};
			}
			else if (inst.instcode == InstStruct::i_load) {
				if (inst._subid == 1) {
					auto &Inst = static_cast<const InstStruct::Insts::Load1&>(inst);

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();

					auto data = Inst.src.data();
					TypeIndex type = Inst.type;

					if (Inst.dst.isPrivateDataRegister()) {
						if (func.is_dyvarb(dst_id)) {
							return [=](Runtime::Environment &env) {
								auto &dst = env.get_dyvarb(dst_id, dst_e);
								auto newdata = data;
								Runtime::DataManage::LoadDataD(env, dst, Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
							};
						}
						else if (func.is_stvarb(dst_id)) {
							return [=](Runtime::Environment &env) {
								auto &dst = env.get_stvarb(dst_id, dst_e);
								auto newdata = data;
								Runtime::DataManage::LoadDataS(env, dst, Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
							};
						}
					}
					else {
						// TODO
						assert(false);
					}
				}
				else if (inst._subid == 2) {
					auto &Inst = static_cast<const InstStruct::Insts::Load2&>(inst);
					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();

					auto index = Inst.src.index();
					TypeIndex type = Inst.type;

					if (Inst.dst.isPrivateDataRegister()) {
						if (func.is_dyvarb(dst_id)) {
							return [=](Runtime::Environment &env) {
								auto &dst = env.get_dyvarb(dst_id, dst_e);
								const auto &pair = env.GEnv().getDataSectionMap().at(index);
								auto &ptr = pair.first;
								auto &size = pair.second;
								Runtime::DataManage::LoadDataD(env, dst, Runtime::ConstDataPointer(ptr), type, MemorySize(size));
							};
						}
						else if (func.is_stvarb(dst_id)) {
							return [=](Runtime::Environment &env) {
								auto &dst = env.get_stvarb(dst_id, dst_e);
								const auto &pair = env.GEnv().getDataSectionMap().at(index);
								auto &ptr = pair.first;
								auto &size = pair.second;
								Runtime::DataManage::LoadDataS(env, dst, Runtime::ConstDataPointer(ptr), type, MemorySize(size));
							};
						}
					}
					else {
						// TODO
						assert(false);
					}
				}
			}
			else if (inst.instcode == InstStruct::i_ret) {
				return [=](Runtime::Environment &env) {
					CheckLocalEnv(env);
					static_cast<Runtime::LocalEnvironment&>(env).Controlflow().setProgramCounterEnd();
				};
			}
			else if (inst.instcode == InstStruct::id_opreg) {
				const auto &typelist = func.stvarb_typelist();
				return [=](Runtime::Environment &env) {
					Config::RegisterIndexType regcount = 0;
					auto &regset = env.getDataRegisterSet();
					PriLib::Output::println("=======================");
					PriLib::Output::println("dyvarb:");
					for (Config::RegisterIndexType i = 1; i <= regset.dysize(); i++) {
						PriLib::Output::print("  %", ++regcount, ": type(", regset.get_dynamic(i).type.data, "), ");
						Runtime::DataManage::Debug_PrintRegisterD(env, regset.get_dynamic(i));
					}
					if (regset.stsize() != 0) {
						PriLib::Output::print("stvarb:");
						//for (size_t i = regset.dysize(); i <= regset.stsize() + regset.stsize());
						//env.getType()
						Runtime::DataPointer address = regset.get_static(regset.dysize() + 1).data;
						printf(" [address : 0x%p]\n", address.get());
						for (Config::RegisterIndexType i = 0; i < typelist.size(); ++i) {
							MemorySize size = env.getType(typelist[i]).size;
							PriLib::Output::print("  %", ++regcount, ": type(", typelist[i].data, "), ");
							const auto &str = Runtime::DataManage::ToStringData(address, size);
							PriLib::Output::println(str);
							address = address.offset(size);
						}
					}
					PriLib::Output::println("=======================");
					PriLib::Output::println();
				};
			}
			println(inst.instcode);
			assert(false);
			return nullptr;
		}

		Runtime::Function Compile(const InstStruct::Function &func) {
			const InstStruct::InstList &src = func.instlist();
			Runtime::Function::InstList dst;

			std::transform(src.begin(), src.end(), std::back_inserter(dst), [&](const InstStruct::Instruction *inst) {
				return Compile(*inst, func);
				});

			return Runtime::Function(dst);
		}

		Runtime::LocalEnvironment* CreateLoaclEnvironment(const InstStruct::Function &func, const TypeInfoMap &tim) {
			// Initialize DataRegisterSet
			Runtime::DataRegisterSet::DyDatRegSize dysize(func.dyvarb_count());
			Runtime::DataRegisterSet::StDatRegSize stsize(func.stvarb_count());

			const auto &typelist = func.stvarb_typelist();
			Runtime::DataRegisterSetStatic::SizeList sizelist(typelist.size());

			size_t i = 0;
			MemorySize size;
			for (auto &type : typelist) {
				MemorySize s = tim.at(type).size;
				sizelist[i++] = s;
				size += s;
			}

			Runtime::DataPointer address = Runtime::DataManage::Alloc(size);

			Runtime::DataRegisterSet drs(dysize, stsize, address, sizelist);

			// Compile Function
			Runtime::Function new_func = Compile(func);

			// Return Environment
			return new Runtime::LocalEnvironment(drs, new_func);
		}

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap &tim, const LiteralDataPool &datasmap) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim, datasmap);
		}
	}
}
