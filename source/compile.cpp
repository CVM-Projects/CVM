#include "basic.h"
#include "compile.h"
#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include "runtime/environment.h"
#include "runtime/datamanage.h"

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

		Runtime::Instruction Compile(const InstStruct::Instruction &inst, const InstStruct::Function &func) {
			if (inst.instcode == InstStruct::i_mov) {
				auto &Inst = static_cast<const InstStruct::Insts::Move&>(inst);

				auto dst_e = Convert(Inst.dst.etype());
				auto src_e = Convert(Inst.src.etype());
				auto dst_id = Inst.dst.index();
				auto src_id = Inst.src.index();

				if (func.is_dyvarb(dst_id) && func.is_dyvarb(src_id)) { // TODO
					return [=](Runtime::Environment &env) {
						auto &dst = env.get_dyvarb(dst_id, dst_e);
						auto &src = env.get_dyvarb(src_id, src_e);
						Runtime::DataManage::MoveRegister(env, dst, src);
					};
				}
				else if (func.is_stvarb(dst_id) && func.is_dyvarb(src_id)) {
					return [=](Runtime::Environment &env) {
						auto &dst = env.get_stvarb(dst_id, dst_e);
						auto &src = env.get_dyvarb(src_id, src_e);
						Runtime::DataManage::MoveRegister(env, dst, src);
					};
				}
				else if (func.is_dyvarb(dst_id) && func.is_stvarb(src_id)) {
					TypeIndex type = func.get_stvarb_type(src_id);
					return [=](Runtime::Environment &env) {
						auto &dst = env.get_dyvarb(dst_id, dst_e);
						auto &src = env.get_stvarb(src_id, src_e);
						Runtime::DataManage::MoveRegister(env, dst, src, type);
					};
				}
				else if (func.is_stvarb(dst_id) && func.is_stvarb(src_id)) {
					TypeIndex type = func.get_stvarb_type(src_id);
					return [=](Runtime::Environment &env) {
						auto &dst = env.get_stvarb(dst_id, dst_e);
						auto &src = env.get_stvarb(src_id, src_e);
						Runtime::DataManage::MoveRegister(env, dst, src, type);
					};
				}
			}
			else if (inst.instcode == InstStruct::i_load) {
				if (inst._subid == 1) {
					auto &Inst = static_cast<const InstStruct::Insts::Load1&>(inst);

					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();

					auto data = Inst.src.data();
					TypeIndex type = Inst.type;

					if (func.is_dyvarb(dst_id)) {
						return [=](Runtime::Environment &env) {
							auto &dst = env.get_dyvarb(dst_id, dst_e);
							auto newdata = data;
							Runtime::DataManage::LoadData(env, dst, Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
						};
					}
					else if (func.is_stvarb(dst_id)) {
						return [=](Runtime::Environment &env) {
							auto &dst = env.get_stvarb(dst_id, dst_e);
							auto newdata = data;
							Runtime::DataManage::LoadData(env, dst, Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
						};
					}
				}
				else if (inst._subid == 2) {
					auto &Inst = static_cast<const InstStruct::Insts::Load2&>(inst);
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
					size_t regcount = 0;
					auto &regset = env.getDataRegisterSet();
					PriLib::Output::println("=======================");
					PriLib::Output::println("dyvarb:");
					for (size_t i = 1; i <= regset.dysize(); i++) {
						PriLib::Output::print("  %", ++regcount, ": type(", regset.get_dynamic(i).type.data, "), ");
						Runtime::DataManage::Debug_PrintRegister(env, regset.get_dynamic(i));
					}
					if (regset.stsize() != 0) {
						PriLib::Output::print("stvarb:");
						//for (size_t i = regset.dysize(); i <= regset.stsize() + regset.stsize());
						//env.getType()
						Runtime::DataPointer address = regset.get_static(regset.dysize() + 1).data;
						printf(" [address : 0x%p]\n", address.get());
						for (size_t i = 0; i < typelist.size(); ++i) {
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
			uint32_t size = 0;
			for (auto &type : typelist) {
				MemorySize s = tim.at(type).size;
				sizelist[i++] = s;
				size += s.data;
			}

			Runtime::DataPointer address = Runtime::DataManage::Alloc(MemorySize(size));

			Runtime::DataRegisterSet drs(dysize, stsize, address, sizelist);

			// Compile Function
			Runtime::Function new_func = Compile(func);

			// Return Environment
			return new Runtime::LocalEnvironment(drs, new_func);
		}

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(size_t dysize, const TypeInfoMap &tim) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim);
		}
	}
}
