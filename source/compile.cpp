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
				return Runtime::DataManage::GetDstData(env.get_dyvarb(index, envtype));
			}
			else if (env.is_stvarb(index, envtype)) {
				return Runtime::DataManage::GetDstData(env.get_stvarb(index, envtype));
			}
			assert(false);
			exit(1);
		}
		Runtime::DataManage::SrcData GetSrcData(Runtime::Environment &env, Config::RegisterIndexType index, Runtime::EnvType envtype, TypeIndex srctype) {
			if (env.is_dyvarb(index, envtype)) {
				return Runtime::DataManage::GetSrcData(env.get_dyvarb(index, envtype));
			}
			else if (env.is_stvarb(index, envtype)) {
				return Runtime::DataManage::GetSrcData(env.get_stvarb(index, envtype), srctype);
			}
			assert(false);
			exit(1);
		}

		void NopeFunc(Runtime::Environment &env) {}

		Runtime::Instruction Compile(const InstStruct::Instruction &inst, const InstStruct::Function &func) {
			if (inst.instcode == InstStruct::i_nop) {
				return NopeFunc;
			}
			else if (inst.instcode == InstStruct::i_mov) {
				auto &Inst = static_cast<const InstStruct::Insts::Move&>(inst);

				if (Inst.dst.isZeroRegister() || Inst.src.isZeroRegister()) {
					println("Error compile with %0.");
					return NopeFunc;
				}

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
						dst_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetDstData(dst_df(env)); };
					}
					else if (func.is_stvarb(dst_id)) {
						TypeIndex type = func.get_stvarb_type(dst_id);
						auto dst_sf = compile_drs(Inst.dst);
						dst_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetDstData(dst_sf(env)); };
					}
				}
				else {
					dst_f = [=](Runtime::Environment &env) { return GetDstData(env, dst_id, dst_e); };
				}

				if (Inst.src.isPrivateDataRegister()) {
					if (func.is_dyvarb(src_id)) {
						auto src_df = compile_drd(Inst.src);
						src_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetSrcData(src_df(env)); };
					}
					else if (func.is_stvarb(src_id)) {
						TypeIndex type = func.get_stvarb_type(src_id);
						auto src_sf = compile_drs(Inst.src);
						src_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetSrcData(src_sf(env), type); };
					}
				}
				else {
					src_f = [=](Runtime::Environment &env) { return GetSrcData(env, src_id, src_e, src_tid); }; // TODO : src_tid
				}

				return [=](Runtime::Environment &env) {
					MoveRegister(env, dst_f(env), src_f(env));
				};
			}
			else if (inst.instcode == InstStruct::i_load) {
				if (inst._subid == 1) {
					auto &Inst = static_cast<const InstStruct::Insts::Load1&>(inst);

					if (Inst.dst.isZeroRegister()) {
						println("Error compile with %0.");
						return NopeFunc;
					}

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();
					TypeIndex type = Inst.type;

					auto data = Inst.src.data();

					if (Inst.dst.isPrivateDataRegister()) {
						return [=](Runtime::Environment &env) {
							auto newdata = data;
							Runtime::DataManage::LoadData(env, GetDstData(env, dst_id, dst_e), Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
						};
					}
					else {
						// TODO
						assert(false);
					}
				}
				else if (inst._subid == 2) {
					auto &Inst = static_cast<const InstStruct::Insts::Load2&>(inst);

					if (Inst.dst.isZeroRegister()) {
						println("Error compile with %0.");
						return NopeFunc;
					}

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();
					TypeIndex type = Inst.type;

					auto index = Inst.src.index();

					if (Inst.dst.isPrivateDataRegister()) {
						return [=](Runtime::Environment &env) {
							const auto &pair = env.GEnv().getDataSectionMap().at(index);
							auto &ptr = pair.first;
							auto &size = pair.second;
							Runtime::DataManage::LoadData(env, GetDstData(env, dst_id, dst_e), Runtime::ConstDataPointer(ptr), type, MemorySize(size));
						};
					}
					else {
						// TODO
						assert(false);
					}
				}
			}
			else if (inst.instcode == InstStruct::i_loadp) {
				if (inst._subid == 1) {
					auto &Inst = static_cast<const InstStruct::Insts::LoadPointer1&>(inst);

					if (Inst.dst.isZeroRegister()) {
						println("Error compile with %0.");
						return NopeFunc;
					}

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();

					auto data = Inst.src.data();

					if (Inst.dst.isPrivateDataRegister()) {
						return [=](Runtime::Environment &env) {
							auto newdata = data;
							Runtime::DataManage::LoadDataPointer(env, GetDstData(env, dst_id, dst_e), Runtime::DataPointer(&newdata), MemorySize(sizeof(InstStruct::Data::Type)));
						};
					}
					else {
						// TODO
						assert(false);
					}
				}
				else if (inst._subid == 2) {
					auto &Inst = static_cast<const InstStruct::Insts::LoadPointer2&>(inst);

					if (Inst.dst.isZeroRegister()) {
						println("Error compile with %0.");
						return NopeFunc;
					}

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();

					auto index = Inst.src.index();

					if (Inst.dst.isPrivateDataRegister()) {
						return [=](Runtime::Environment &env) {
							const auto &pair = env.GEnv().getDataSectionMap().at(index);
							auto &ptr = pair.first;
							auto &size = pair.second;
							Runtime::DataManage::LoadDataPointer(env, GetDstData(env, dst_id, dst_e), Runtime::ConstDataPointer(ptr), MemorySize(size));
						};
					}
					else {
						// TODO
						assert(false);
					}
				}
			}
			else if (inst.instcode == InstStruct::i_call) {
				auto &Inst = static_cast<const InstStruct::Insts::Call&>(inst);
				std::function<Runtime::DataManage::DstData(Runtime::Environment &env)> dst_f;
				if (Inst.dst.isZeroRegister()) {
					dst_f = [](Runtime::Environment &env) {
						return Runtime::DataManage::DstData();
					};
				}
				else if (Inst.dst.isPrivateDataRegister()) {
					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();
					dst_f = [=](Runtime::Environment &env) {
						return GetDstData(env, dst_id, dst_e);
					};
				}
				else {
					assert(false);
				}
				std::function<Runtime::Function*(Runtime::Environment &env)> f;
				const std::string &name = Inst.func.data();
				f = [=](Runtime::Environment &env) -> Runtime::Function* {
					auto &table = env.GEnv().getFuncTable();
					return table.getValue(table.findKey(name));
				};
				std::vector<std::function<Runtime::DataManage::SrcData(Runtime::Environment &env)>> src_fs;
				for (auto &arg : Inst.arglist.data()) {
					if (arg.isPrivateDataRegister()) {
						auto src_e = Convert(arg.etype());
						auto src_id = arg.index();
						TypeIndex type;
						if (func.is_stvarb(src_id))
							type = func.get_stvarb_type(src_id);
						src_fs.push_back([=](Runtime::Environment &env) {
							return GetSrcData(env, src_id, src_e, type);
						});
					}
					else {
						assert(false);
					}
				}
				return [=](Runtime::Environment &env) {
					const Runtime::Function &ff = *f(env);
					PriLib::lightlist_creater<Runtime::DataManage::SrcData> arglist_creater(src_fs.size());
					for (auto &src_f : src_fs) {
						arglist_creater.push_back(src_f(env));
					}

					Runtime::DataManage::Call(env, ff, dst_f(env), arglist_creater.data());
				};
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
						PriLib::Output::print("  %", ++regcount, ": type(", env.getType(regset.get_dynamic(i).type).name.data, "), ");
						Runtime::DataManage::Debug_PrintRegister(env, regset.get_dynamic(i));
					}
					if (regset.stsize() != 0) {
						PriLib::Output::print("stvarb:");
						//for (size_t i = regset.dysize(); i <= regset.stsize() + regset.stsize());
						//env.getType()
						Runtime::DataPointer address = regset.get_static(regset.dysize() + 1).data;
						printf(" [address : 0x%p]\n", address.get());
						for (Config::RegisterIndexType i = 0; i < typelist.size(); ++i) {
							MemorySize size = env.getType(typelist[i]).size;
							PriLib::Output::print("  %", ++regcount, ": type(", env.getType(typelist[i]).name.data, "), ");
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

		Runtime::InstFunction Compile(const InstStruct::Function &func) {
			const InstStruct::InstList &src = func.instlist();
			Runtime::InstFunction::InstList dst;

			std::transform(src.begin(), src.end(), std::back_inserter(dst), [&](const InstStruct::Instruction *inst) {
				return Compile(*inst, func);
				});

			return Runtime::InstFunction(dst, &func);
		}

		Runtime::LocalEnvironment* CreateLoaclEnvironment(const Runtime::InstFunction &func, const TypeInfoMap &tim) {
			const auto &ifunc = func.instfunc();

			// Initialize DataRegisterSet
			Runtime::DataRegisterSet::DyDatRegSize dysize(ifunc.dyvarb_count());
			Runtime::DataRegisterSet::StDatRegSize stsize(ifunc.stvarb_count());

			const auto &typelist = ifunc.stvarb_typelist();
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

			// Return Environment
			return new Runtime::LocalEnvironment(drs, func);
		}

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap &tim, const LiteralDataPool &datasmap, const Runtime::FuncTable &functable) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim, datasmap, functable);
		}
	}
}
