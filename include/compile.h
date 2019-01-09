#pragma once
#include "inststruct/instruction.h"
#include "inststruct/function.h"
#include "runtime/instruction.h"
#include "runtime/function.h"
#include "runtime/environment.h"
#include "virtualmachine.h"
#include "parse.h"

namespace CVM
{
	class Compiler
	{
	public:
		explicit Compiler() {}

		bool compile(ParseInfo &parseinfo, const Runtime::PtrFuncMap &pfm, Runtime::FuncTable &functable);

		Config::FuncIndexType getEntryID() {
			return entry_index;
		}

	private:
		Runtime::Instruction* compile(const InstStruct::Instruction &inst, const FunctionInfo &info);
		Runtime::InstFunction compile(const InstStruct::Function &func);
		Config::FuncIndexType entry_index;
	};

	namespace Compile
	{
		Runtime::LocalEnvironment* CreateLoaclEnvironment(const Runtime::InstFunction &func, const TypeInfoMap &tim);
		Runtime::GlobalEnvironment* CreateGlobalEnvironment(Config::RegisterIndexType dysize, const TypeInfoMap *tim, const LiteralDataPool *datasmap, const Runtime::FuncTable *functable, HashStringPool *hashStringPool);
	}
}
