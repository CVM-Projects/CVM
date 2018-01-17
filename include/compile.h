#pragma once
#include "instruction/instruction.h"
#include "instruction/function.h"
#include "runtime/instruction.h"
#include "runtime/function.h"
#include "runtime/environment.h"
#include "virtualmachine.h"

namespace CVM
{
	namespace Compile
	{
		Runtime::Instruction Compile(const Instruction::Instruction &inst, const Instruction::Function &func);
		Runtime::Function Compile(const Instruction::Function &func);

		Runtime::LocalEnvironment* CreateLoaclEnvironment(const Instruction::Function &func, const TypeInfoMap &tim);
		Runtime::GlobalEnvironment* CreateGlobalEnvironment(size_t dysize, const TypeInfoMap &tim);
	}
}
