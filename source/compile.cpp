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

		struct ResultDataGetFunc
		{
			int mode = 0;
			Config::RegisterIndexType dst_id = 0;

			Runtime::DataManage::ResultData call(Runtime::Environment &env) const {
				switch (mode)
				{
				case 0:
					return Runtime::DataManage::ResultData { Runtime::rt_null, nullptr };
				case 1:
					if (env.is_dyvarb(dst_id, Runtime::e_current))
						return Runtime::DataManage::ResultData { Runtime::rt_dynamic, &env.get_dyvarb(dst_id, Runtime::e_current) };
					else
						return Runtime::DataManage::ResultData { Runtime::rt_static, &env.get_stvarb(dst_id, Runtime::e_current) };
				case 2:
					return Runtime::DataManage::ResultData { env.get_result().rtype, env.get_result().drp };
				default:
					assert(false);
					return Runtime::DataManage::ResultData {};
				}
			}
		};
		struct SrcDataGetFunc
		{
			TypeIndex type;
			Config::RegisterIndexType src_id = 0;

			Runtime::DataManage::SrcData call(Runtime::Environment &env) const {
				return GetSrcData(env, src_id, Runtime::e_current, type);
			}
		};
	}

	Runtime::Instruction Compiler::compile(const InstStruct::Instruction &inst, const FunctionInfo &info) {
		using namespace Compile;

		if (inst.instcode == InstStruct::i_nop) {
			return NopeFunc;
		}
		else if (inst.instcode == InstStruct::i_mov) {
			auto &Inst = static_cast<const InstStruct::Insts::Move&>(inst);

			if (Inst.dst.isZeroRegister() || Inst.src.isZeroRegister()) {
				println("Error compile with %0.");
				return NopeFunc;
			}

			using namespace Runtime::DataManage;
			DstData dst;

			std::function<DstData(Runtime::Environment &)> dst_f;

			if (Inst.dst.isResultRegister()) {
				dst_f = Runtime::DataManage::GetDstDataResult;
			}
			else {
				auto dst_e = Convert(Inst.dst.etype());
				auto dst_id = Inst.dst.index();;

				if (Inst.dst.isPrivateDataRegister()) {
					if (info.is_dyvarb(dst_id)) {
						auto dst_df = compile_drd(Inst.dst);
						dst_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetDstData(dst_df(env)); };
					}
					else if (info.is_stvarb(dst_id)) {
						TypeIndex type = info.get_stvarb_type(dst_id);
						auto dst_sf = compile_drs(Inst.dst);
						dst_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetDstData(dst_sf(env)); };
					}
				}
				else {
					dst_f = [=](Runtime::Environment &env) { return GetDstData(env, dst_id, dst_e); };
				}
			}

			auto src_e = Convert(Inst.src.etype());
			auto src_id = Inst.src.index();

			TypeIndex src_tid = Inst.src.have_tindex() ? Inst.src.tindex() : TypeIndex(0);
			SrcData src;
			std::function<SrcData(Runtime::Environment &)> src_f;

			if (Inst.src.isPrivateDataRegister()) {
				if (info.is_dyvarb(src_id)) {
					auto src_df = compile_drd(Inst.src);
					src_f = [=](Runtime::Environment &env) { return Runtime::DataManage::GetSrcData(src_df(env)); };
				}
				else if (info.is_stvarb(src_id)) {
					TypeIndex type = info.get_stvarb_type(src_id);
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

				TypeIndex type = Inst.type;

				auto data = Inst.src.data();

				if (Inst.dst.isPrivateDataRegister()) {
					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();
					return [=](Runtime::Environment &env) {
						auto newdata = data;
						Runtime::DataManage::LoadData(env, GetDstData(env, dst_id, dst_e), Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
					};
				}
				else if (Inst.dst.isResultRegister()) {
					return [=](Runtime::Environment &env) {
						auto newdata = data;
						Runtime::DataManage::LoadData(env, Runtime::DataManage::GetDstDataResult(env), Runtime::DataPointer(&newdata), type, MemorySize(sizeof(InstStruct::Data::Type)));
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

				TypeIndex type = Inst.type;

				auto index = Inst.src.index();

				if (Inst.dst.isPrivateDataRegister()) {
					auto dst_e = Convert(Inst.dst.etype());
					auto dst_id = Inst.dst.index();
					return [=](Runtime::Environment &env) {
						const auto &pair = env.GEnv().getDataSectionMap().at(index);
						auto &ptr = pair.first;
						auto &size = pair.second;
						Runtime::DataManage::LoadData(env, GetDstData(env, dst_id, dst_e), Runtime::ConstDataPointer(ptr), type, MemorySize(size));
					};
				}
				else if (Inst.dst.isResultRegister()) {
					return [=](Runtime::Environment &env) {
						const auto &pair = env.GEnv().getDataSectionMap().at(index);
						auto &ptr = pair.first;
						auto &size = pair.second;
						Runtime::DataManage::LoadData(env, Runtime::DataManage::GetDstDataResult(env), Runtime::ConstDataPointer(ptr), type, MemorySize(size));
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
			ResultDataGetFunc dst_f;

			if (Inst.dst.isZeroRegister()) {
				dst_f.mode = 0;
			}
			else if (Inst.dst.isPrivateDataRegister()) {
				auto dst_id = Inst.dst.index();
				dst_f.mode = 1;
				dst_f.dst_id = dst_id;
			}
			else if (Inst.dst.isResultRegister()) {
				dst_f.mode = 2;
			}
			else {
				assert(false);
			}
			const Config::FuncIndexType &id = Inst.func.data();
			std::vector<SrcDataGetFunc> src_fs;
			for (auto &arg : Inst.arglist.data()) {
				if (arg.isPrivateDataRegister()) {
					auto src_id = arg.index();
					TypeIndex type;
					if (info.is_stvarb(src_id))
						type = info.get_stvarb_type(src_id);
					SrcDataGetFunc sdgf;
					sdgf.src_id = src_id;
					sdgf.type = type;
					src_fs.push_back(sdgf);
				}
				else {
					assert(false);
				}
			}

			auto fx = [=](Runtime::Environment &env) {
				auto &table = env.GEnv().getFuncTable();
				auto f = table.at(id);

				const Runtime::Function &ff = *f;
				PriLib::lightlist_creater<Runtime::DataManage::SrcData> arglist_creater(src_fs.size());
				for (auto &src_f : src_fs) {
					arglist_creater.push_back(src_f.call(env));
				}

				Runtime::DataManage::ResultData rd = dst_f.call(env);
				Runtime::DataManage::Call(env, ff, rd, arglist_creater.data());
			};

			sizeof(fx);

			return fx;
		}
		else if (inst.instcode == InstStruct::i_ret) {
			return [=](Runtime::Environment &env) {
				CheckLocalEnv(env);
				static_cast<Runtime::LocalEnvironment&>(env).Controlflow().setProgramCounterEnd();
			};
		}
		else if (inst.instcode == InstStruct::id_opreg) {
			const auto &typelist = info.sttypelist();
			auto typelist_count = info.stvarb_count();
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
					for (Config::RegisterIndexType i = 0; i < typelist_count; ++i) {
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
}

#include "parse.h"
#include "inststruct/identkeytable.h"

namespace CVM
{
	Runtime::InstFunction Compiler::compile(const InstStruct::Function &func) {
		const InstStruct::InstList &src = func.instdata;
		Runtime::InstFunction::InstList dst;
		FunctionInfo&info = const_cast<FunctionInfo&>(func.info); // TODO

		std::transform(src.begin(), src.end(), std::back_inserter(dst), [&](const InstStruct::Instruction *inst) {
			return compile(*inst, info);
			});

		return Runtime::InstFunction(std::move(dst), std::move(info));
	}

	bool Compiler::compile(ParseInfo &parseinfo, const Runtime::PtrFuncMap &pfm, Runtime::FuncTable &functable) {
		InstStruct::IdentKeyTable &ikt = getFunctionTable(parseinfo);

		// Get entry func

		auto entry = getEntry(parseinfo);
		if (entry.empty()) {
			println("Undeclared entry function.");
			return false;
		}
		if (!ikt.hasKey(entry)) {
			println("Not find '" + entry + "' function.");
			return false;
		}

		this->entry_index = ikt.getID(entry);

		// Compile All Functions

		ikt.each([&](Config::FuncIndexType id, const InstStruct::IdentKeyTable::FuncPtr &f) {
			if (f) {
				Runtime::Function *fp = new Runtime::InstFunction(compile(*f));
				functable.insert({ id, fp });
			}});

		for (auto &pair : pfm) {
			Config::FuncIndexType id = ikt.getID(pair.first);
			functable.insert({ id, new Runtime::PointerFunction(pair.second) });
		}

		return true;
	}
}

namespace CVM
{
	// TODO : Move these code to other file.
	namespace Compile
	{
		Runtime::LocalEnvironment* CreateLoaclEnvironment(const Runtime::InstFunction &func, const TypeInfoMap &tim) {
			const auto &info = func.info();

			// Initialize DataRegisterSet
			Runtime::DataRegisterSet::DyDatRegSize dysize(info.dyvarb_count());
			Runtime::DataRegisterSet::StDatRegSize stsize(info.stvarb_count());

			const auto &typelist = info.sttypelist();
			Runtime::DataRegisterSetStatic::SizeList sizelist(info.stvarb_count());

			size_t i = 0;
			MemorySize size;
			for (Config::RegisterIndexType i = 0; i != info.stvarb_count(); ++i) {
				const TypeIndex& type = typelist[i];
				MemorySize s = tim.at(type).size;
				sizelist[i] = s;
				size += s;
			}

			Runtime::DataPointer address = Runtime::DataManage::Alloc(size);

			Runtime::DataRegisterSet drs(dysize, stsize, address, sizelist);

			// Return Environment
			return new Runtime::LocalEnvironment(drs, func);
		}

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap *tim, const LiteralDataPool *datasmap, const Runtime::FuncTable *functable) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim, datasmap, functable);
		}
	}
}
