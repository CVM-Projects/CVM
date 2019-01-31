#include "basic.h"
#include "compiler/compile.h"
#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include "runtime/environment.h"
#include "runtime/datamanage.h"
#include "datapool.h"
#include "runtime/instdef.hpp"

namespace CVM
{
	namespace Compile
	{
		inline static void CheckLocalEnv(Runtime::Environment &env) {
			if (!env.isLocal()) {
				puts("Error Occur because of not LocalEnvironment.");
				exit(-1);
			}
		}
	}

	namespace Compile {
		static Runtime::Instruction* const NopeInst = new Runtime::Insts::Nope();

		static Runtime::Instruction* compile_Move(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			auto &Inst = static_cast<const InstStruct::Insts::Move&>(inst);

			if (Inst.dst.isZeroRegister() || Inst.src.isZeroRegister()) {
				println("Error compile with %0.");
				return NopeInst;
			}

			if (Inst.dst.isPrivateDataRegister() && Inst.src.isPrivateDataRegister()) {
				auto dst_id = Inst.dst.index();
				auto src_id = Inst.src.index();

				if (info.is_dyvarb(dst_id) && info.is_dyvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDdDd(dst_id, src_id);
				}
				else if (info.is_stvarb(dst_id) && info.is_dyvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDsDd(dst_id, src_id);
				}
				else if (info.is_dyvarb(dst_id) && info.is_stvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDdDs(dst_id, src_id, info.get_stvarb_type(src_id));
				}
				else if (info.is_stvarb(dst_id) && info.is_stvarb(src_id)) {
					if (info.get_stvarb_type(dst_id).data != info.get_stvarb_type(src_id).data) {
						println("Error must be same type with two static data registers.");
						return NopeInst;
					}
					return new Runtime::Insts::MoveRegisterDsDs(dst_id, src_id, info.get_stvarb_type(src_id));
				}
			}
			else if (Inst.dst.isResultRegister() && Inst.src.isPrivateDataRegister()) {
				auto src_id = Inst.src.index();

				if (info.is_dyvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDdRes(src_id, info.get_accesser().result_type());
				}
				else if (info.is_stvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDsRes(src_id, info.get_accesser().result_type());
				}
			}
			else if (Inst.dst.isPrivateDataRegister() && Inst.src.isResultRegister()) {
				auto dst_id = Inst.dst.index();

				if (info.is_dyvarb(dst_id)) {
					return new Runtime::Insts::MoveRegisterResDd(dst_id);
				}
				else if (info.is_stvarb(dst_id)) {
					return new Runtime::Insts::MoveRegisterResDs(dst_id, info.get_stvarb_type(dst_id));
				}
			}
			else {
				assert(false);
			}

			return NopeInst;
		}

		template <typename LoadTy>
		static bool check_Load_dst_Base(const LoadTy &inst) {
			if (inst.dst.isZeroRegister()) {
				println("Error compile with %0.");
				return false;
			}
			return true;
		}

		static Runtime::Instruction* compile_Load(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			if (inst._subid == 1) {
				auto &Inst = static_cast<const InstStruct::Insts::Load1&>(inst);
				if (!check_Load_dst_Base(Inst)) {
					return NopeInst;
				}

				TypeIndex type = Inst.type;

				auto data = Inst.src.data();

				if (Inst.dst.isPrivateDataRegister()) {
					auto dst_id = Inst.dst.index();
					if (info.is_dyvarb(dst_id)) {
						return new Runtime::Insts::LoadDataDd<1>(dst_id, type, data);
					}
					else if (info.is_stvarb(dst_id)) {
						// TODO!!! type & dsttype is different !
						return new Runtime::Insts::LoadDataDs<1>(dst_id, type, data);
					}
					else {
						assert(false);
					}
				}
				else if (Inst.dst.isResultRegister()) {
					// TODO!!! type & restype is different !
					return new Runtime::Insts::LoadDataRes<1>(type, data);
				}
				else {
					assert(false);
				}
			}
			else if (inst._subid == 2) {
				auto &Inst = static_cast<const InstStruct::Insts::Load2&>(inst);
				if (!check_Load_dst_Base(Inst)) {
					return NopeInst;
				}

				TypeIndex type = Inst.type;

				auto index = Inst.src.index();

				if (Inst.dst.isPrivateDataRegister()) {
					auto dst_id = Inst.dst.index();
					if (info.is_dyvarb(dst_id)) {
						return new Runtime::Insts::LoadDataDd<2>(dst_id, type, index);
					}
					else if (info.is_stvarb(dst_id)) {
						// TODO!!! type & dsttype is different !
						return new Runtime::Insts::LoadDataDs<2>(dst_id, type, index);
					}
					else {
						assert(false);
					}
				}
				else if (Inst.dst.isResultRegister()) {
					// TODO!!! type & restype is different !
					return new Runtime::Insts::LoadDataRes<2>(type, index);
				}
				else {
					assert(false);
				}
			}
			else {
				assert(false);
			}

			return NopeInst;
		}

		static Runtime::Instruction* compile_LoadPointer(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			auto &Inst = static_cast<const InstStruct::Insts::LoadPointer&>(inst);
			if (!check_Load_dst_Base(Inst)) {
				return NopeInst;
			}

			auto index = Inst.src.index();

			if (Inst.dst.isPrivateDataRegister()) {
				auto dst_id = Inst.dst.index();
				if (info.is_dyvarb(dst_id)) {
					return new Runtime::Insts::LoadDataPointerDd(dst_id, index);
				}
				else if (info.is_stvarb(dst_id)) {
					const auto &type = info.get_stvarb_type(dst_id);
					return new Runtime::Insts::LoadDataPointerDs(dst_id, type, index);
				}
				else {
					assert(false);
				}
			}
			else if (Inst.dst.isResultRegister()) {
				const auto &type = info.get_accesser().result_type();
				return new Runtime::Insts::LoadDataPointerRes(type, index);
			}
			else {
				assert(false);
			}

			return NopeInst;
		}

		static Runtime::Instruction* compile_Call(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			auto &Inst = static_cast<const InstStruct::Insts::Call&>(inst);

			Config::FuncIndexType fid = Inst.func.data();

			Runtime::Insts::Call::ArgListType::creater arglist_creater(Inst.arglist.data().size());

			// TODO : More Compile Action
			for (auto &arg : Inst.arglist.data()) {
				if (arg.isPrivateDataRegister()) {
					auto index = arg.index();
					if (info.is_dyvarb(index) || info.is_stvarb(index)) {
						arglist_creater.push_back(arg.index());
					}
					else {
						assert(false);
						return NopeInst;
					}
				}
				else {
					assert(false);
					return NopeInst;
				}
			}

			if (Inst.dst.isPrivateDataRegister()) {
				auto index = Inst.dst.index();
				assert(info.is_dyvarb(index) || info.is_stvarb(index));

				return new Runtime::Insts::CallDds(index, fid, arglist_creater.data());
			}
			else if (Inst.dst.isResultRegister()) {
				return new Runtime::Insts::CallRes(fid, arglist_creater.data());
			}
			else if (Inst.dst.isZeroRegister()) {
				return new Runtime::Insts::Call(fid, arglist_creater.data());
			}
			else {
				assert(false);
			}

			return NopeInst;
		}
	}

	Runtime::Instruction* Compiler::compile(const InstStruct::Instruction &inst, const FunctionInfo &info) {
		using namespace Compile;

		if (inst.instcode == InstStruct::i_nop) {
			return NopeInst;
		}
		else if (inst.instcode == InstStruct::i_mov) {
			return compile_Move(inst, info);
		}
		else if (inst.instcode == InstStruct::i_load) {
			return compile_Load(inst, info);
		}
		else if (inst.instcode == InstStruct::i_loadp) {
			return compile_LoadPointer(inst, info);
		}
		else if (inst.instcode == InstStruct::i_ret) {
			return new Runtime::Insts::Return();
		}
		else if (inst.instcode == InstStruct::i_jump) {
			auto &Inst = static_cast<const InstStruct::Insts::Jump&>(inst);
			return new Runtime::Insts::Jump(Inst.line);
		}
		else if (inst.instcode == InstStruct::i_call) {
			return compile_Call(inst, info);
		}
		else if (inst.instcode == InstStruct::id_opreg) {
			return new Runtime::InstsDebug::OutputRegister();
		}
		else {
			println(inst.instcode);
			return NopeInst;
		}

		assert(false);

		return NopeInst;
	}
}

#include "parser/parse.h"
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

		auto entry = getFromHashStringPool(parseinfo, getEntry(parseinfo));
		if (entry.empty()) {
			println("Undeclared entry function.");
			return false;
		}
		if (!ikt.hasKey(getEntry(parseinfo))) {
			println("Not find '" + entry + "' function.");
			return false;
		}

		this->entry_index = ikt.getID(getEntry(parseinfo));

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

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap *tim, const NewLiteralDataPool *datasmap, const Runtime::FuncTable *functable, HashStringPool *hashStringPool) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim, datasmap, functable, hashStringPool);
		}
	}
}
