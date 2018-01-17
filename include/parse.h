#pragma once
#include "Instruction/instruction.h"

namespace CVM
{
	enum ParseLineType
	{

	};

	class Match;

	Instruction::Instruction* parseInstruction();
	Instruction::Instruction* parseFunction();
}
