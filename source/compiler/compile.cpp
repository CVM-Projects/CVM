#include "basic.h"
#include "compiler/compile.h"
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

	// Temp func
	static bool check(const std::vector<InstStruct::Element> &list, const std::vector<InstStruct::ElementType> &et) {
		if (list.size() != et.size())
			return false;
		for (size_t i = 0; i != list.size(); ++i) {
			if (list[i].type() != et[i])
				return false;
		}
		return true;
	}

	namespace Compile {
		static Runtime::Instruction* const NopeInst = new Runtime::Insts::Nope();

		static Runtime::Instruction* compile_Move(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			if (!check(inst.data, { InstStruct::ET_Register, InstStruct::ET_Register }))
				println("Error type for inst");

			auto &dst = inst.data[0].get<InstStruct::Register>();
			auto &src = inst.data[1].get<InstStruct::Register>();

			if (dst.isZeroRegister() || src.isZeroRegister()) {
				println("Error compile with %0.");
				return NopeInst;
			}

			if (dst.isPrivateDataRegister() && src.isPrivateDataRegister()) {
				auto dst_id = dst.index();
				auto src_id = src.index();

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
			else if (dst.isResultRegister() && src.isPrivateDataRegister()) {
				auto src_id = src.index();

				if (info.is_dyvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDdRes(src_id, info.get_accesser().result_type());
				}
				else if (info.is_stvarb(src_id)) {
					return new Runtime::Insts::MoveRegisterDsRes(src_id, info.get_accesser().result_type());
				}
			}
			else if (dst.isPrivateDataRegister() && src.isResultRegister()) {
				auto dst_id = dst.index();

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

		static bool check_dst_is_not_zero(const InstStruct::Instruction &inst) {
			if (inst.data[0].get<InstStruct::Register>().isZeroRegister()) {
				println("Error compile with %0.");
				return false;
			}
			return true;
		}
		
		// TEMP
		TypeInfoMap *_ptypeInfoMap = nullptr;  // TODO!!
		InstStruct::IdentKeyTable *_pfuncTable = nullptr;  // TODO!!

		static TypeIndex parseType(TypeInfoMap &typeInfoMap, const InstStruct::Element &elt) {
			TypeIndex index;
			if (typeInfoMap.find(elt.get<InstStruct::Identifier>().data(), index)) {
				return index;
			}
			else {
				println("Type error");
				return TypeIndex(0);
			}
		}

		// TODO
		static uint32_t parseData(const InstStruct::Element &elt) {
			auto &data = elt.get<InstStruct::IntegerData>();
			if (data.data().size() <= sizeof(uint32_t)) {
				uint32_t result = 0;
				if (data.data().toBuffer(&result, sizeof(result)))
					return result;
			}
			println("Error parse uint32 data.");
			return 0;
		}

		static Runtime::Instruction* compile_Load(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			if (check(inst.data, { InstStruct::ET_Register, InstStruct::ET_IntegerData, InstStruct::ET_Identifier })) {

				if (!check_dst_is_not_zero(inst)) {
					return NopeInst;
				}

				TypeIndex type = parseType(*_ptypeInfoMap, inst.data[2]);

				auto data = parseData(inst.data[1]);

				auto &dst = inst.data[0].get<InstStruct::Register>();

				if (dst.isPrivateDataRegister()) {
					auto dst_id = dst.index();
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
				else if (dst.isResultRegister()) {
					// TODO!!! type & restype is different !
					return new Runtime::Insts::LoadDataRes<1>(type, data);
				}
				else {
					assert(false);
				}
			}
			else if (check(inst.data, { InstStruct::ET_Register, InstStruct::ET_DataLabel, InstStruct::ET_Identifier })) {

				if (!check_dst_is_not_zero(inst)) {
					return NopeInst;
				}

				TypeIndex type = parseType(*_ptypeInfoMap, inst.data[2]);

				auto &src = inst.data[1].get<InstStruct::DataLabel>();
				auto &dst = inst.data[0].get<InstStruct::Register>();

				auto index = src.data();

				if (dst.isPrivateDataRegister()) {
					auto dst_id = dst.index();
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
				else if (dst.isResultRegister()) {
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
			if (!check(inst.data, { InstStruct::ET_Register, InstStruct::ET_DataLabel }))
				return NopeInst;

			if (!check_dst_is_not_zero(inst)) {
				return NopeInst;
			}

			auto &src = inst.data[1].get<InstStruct::DataLabel>();
			auto &dst = inst.data[0].get<InstStruct::Register>();

			auto index = src.data();

			if (dst.isPrivateDataRegister()) {
				auto dst_id = dst.index();
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
			else if (dst.isResultRegister()) {
				const auto &type = info.get_accesser().result_type();
				return new Runtime::Insts::LoadDataPointerRes(type, index);
			}
			else {
				assert(false);
			}

			return NopeInst;
		}

		static Runtime::Instruction* compile_Call(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			Config::FuncIndexType fid = 0;
			InstStruct::ArgumentList arglist;
			InstStruct::Register dst;

			if (inst.data.size() >= 2) {
				if (inst.data[0].type() == InstStruct::ET_Register && inst.data[1].type() == InstStruct::ET_Identifier) {
					auto res = inst.data[0].get<InstStruct::Register>();
					const auto &namekey = inst.data[1].get<InstStruct::Identifier>().data();
					auto id = _pfuncTable->getID(namekey);
					InstStruct::FuncIdentifier func(id);
					InstStruct::ArgumentList::creater arglist_creater(inst.data.size() - 2);
					for (auto &e : PriLib::rangei(inst.data.begin() + 2, inst.data.end())) {
						if (e.type() != InstStruct::ET_Register) {
							println("Not register");
							return NopeInst;
						}
						arglist_creater.push_back(e.get<InstStruct::Register>());
					}
					fid = func.data;
					arglist = arglist_creater.data();
					dst = res;
				}
			}
			else {
				return NopeInst;
			}

			Runtime::Insts::Call::ArgListType::creater arglist_creater(arglist.size());

			// TODO : More Compile Action
			for (auto &arg : arglist) {
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

			if (dst.isPrivateDataRegister()) {
				auto index = dst.index();
				assert(info.is_dyvarb(index) || info.is_stvarb(index));

				return new Runtime::Insts::CallDds(index, fid, arglist_creater.data());
			}
			else if (dst.isResultRegister()) {
				return new Runtime::Insts::CallRes(fid, arglist_creater.data());
			}
			else if (dst.isZeroRegister()) {
				return new Runtime::Insts::Call(fid, arglist_creater.data());
			}
			else {
				assert(false);
			}

			return NopeInst;
		}

		InstStruct::LabelKeyTable *labelkeytable = nullptr;   // TODO

		static Runtime::Instruction* compile_Jump(const InstStruct::Instruction &inst, const FunctionInfo &info) {
			if (check(inst.data, { InstStruct::ET_LineLabel })) {
				InstStruct::LineLabel label = inst.data[0].get<InstStruct::LineLabel>();
				size_t line = (*labelkeytable)[label.data()];
				return new Runtime::Insts::Jump(static_cast<Config::LineCountType>(line));
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
			return compile_Jump(inst, info);
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

		CVM::Compile::labelkeytable = (InstStruct::LabelKeyTable*)&func.labelkeytable; // TODO

		std::transform(src.begin(), src.end(), std::back_inserter(dst), [&](const InstStruct::Instruction *inst) {
			return compile(*inst, info);
		});

		return Runtime::InstFunction(std::move(dst), std::move(info));
	}

	bool Compiler::compile(InstStruct::GlobalInfo &globalinfo, const Runtime::PtrFuncMap &pfm, Runtime::FuncTable &functable) {
		InstStruct::IdentKeyTable &ikt = globalinfo.funcTable;

		// Get entry func

		if (!globalinfo.hashStringPool.has(globalinfo.entry)) {
			println("Undeclared entry function.");
			return false;
		}
		if (!ikt.hasKey(globalinfo.entry)) {
			auto entry = globalinfo.hashStringPool.get(globalinfo.entry);
			println("Not find '" + entry + "' function.");
			return false;
		}

		this->entry_index = ikt.getID(globalinfo.entry);

		// TODO
		Compile::_ptypeInfoMap = &globalinfo.typeInfoMap;  // TODO!!
		Compile::_pfuncTable = &globalinfo.funcTable;  // TODO!!

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

		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap *tim, const LiteralDataPool *datasmap, const Runtime::FuncTable *functable, HashStringPool *hashStringPool) {
			Runtime::DataRegisterSet::DyDatRegSize _dysize(dysize);
			Runtime::DataRegisterSet drs(_dysize);

			// Return Environment
			return new Runtime::GlobalEnvironment(drs, tim, datasmap, functable, hashStringPool);
		}
	}
}
