#include "basic.h"
#include "runtime/environment.h"
#include "virtualmachine.h"
using namespace PRILIB;
using Output::print;
using Output::println;
using Output::putError;
#include "typeinfo.h"
#include "runtime/datapointer.h"

int add_int(int x, int y) {
	return x + y;
}

struct Point
{
	char x;
	int y;
};

Point add_Point(Point x, Point y)
{
	return Point { x.x + y.x, x.y + y.y };
}

#include "runtime/datamanage.h"

string to_string_data(CVM::Runtime::DataPointer dp, CVM::MemorySize size) {
	return CVM::Runtime::DataManage::ToStringData(dp, size);
}

void print_data(CVM::Runtime::DataPointer dp, CVM::MemorySize size) {
	println(to_string_data(dp, size));
}

using CVM::Runtime::DataPointer;

namespace CVM
{
	void VirtualMachine::Call(Runtime::LocalEnvironment &env) {
		auto &cflow = env.Controlflow();
		while (cflow.isInstRunning()) {
			cflow.init();
			cflow.callCurrInst(env);
			cflow.incProgramCounter();
		}
	}
}

#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include "typeinfo.h"
#include "compile.h"

CVM::InstStruct::Function CreateFunction_Main() {
	using namespace CVM::InstStruct;
	using namespace Insts;

	InstList instlist;

	// main:
	//     .arg    0
	//     .dyvarb 2
	//     .stvarb %3 %4 %5 %6, int
	//     db_opreg
	//     load %1, 5, int
	//     db_opreg
	//     mov  %2, %1
	//     mov  %3, %1
	//     mov  %4, %1
	//     db_opreg
	//     load %5, 6, int
	//     db_opreg
	//     ret

	instlist.push_back(new Debug_OutputRegister());
	instlist.push_back(new Load1(Register(r_n, e_current, 1), Data(5), CVM::TypeIndex(1)));
	instlist.push_back(new Debug_OutputRegister());
	instlist.push_back(new Move(Register(r_n, e_current, 2), Register(r_n, e_current, 1)));
	instlist.push_back(new Move(Register(r_n, e_current, 3), Register(r_n, e_current, 1)));
	instlist.push_back(new Move(Register(r_n, e_current, 4), Register(r_n, e_current, 1)));
	instlist.push_back(new Debug_OutputRegister());
	instlist.push_back(new Load1(Register(r_n, e_current, 5), Data(6), CVM::TypeIndex(1)));
	instlist.push_back(new Debug_OutputRegister());
	instlist.push_back(new Return());

	TypeList typelist = { CVM::TypeIndex(1), CVM::TypeIndex(1), CVM::TypeIndex(1), CVM::TypeIndex(1) };

	return Function(FunctionInfo(std::move(instlist), 2, std::move(typelist), ArgList {}));
}

#include "parse.h"

int main()
{
	TextFile cmsfile;

	cmsfile.open("test.cms", File::Read);

	if (cmsfile.bad()) {
		putError("Error in open file.");
	}
	
	auto parseInfo = CVM::createParseInfo();
	parseFile(parseInfo, cmsfile);

	return 0;

	using namespace CVM;

	VirtualMachine VM;

	TypeInfoMap tim;

	TypeInfo ti_nil;

	ti_nil.index.data = 0;
	ti_nil.name.data = "nil";
	ti_nil.size.data = 0;

	TypeInfo ti_int;

	ti_int.index.data = 1;
	ti_int.name.data = "int";
	ti_int.size.data = sizeof(int);

	DataPointer dp1(Memory::alloc(ti_int.size.data));
	int &i = *dp1.get<int>();
	i = 10;
	println(add_int(i, i));

	TypeInfo ti_Point;

	ti_Point.index.data = 2;
	ti_Point.name.data = "Point";
	ti_Point.size.data = sizeof(Point);

	DataPointer dp2(Memory::alloc(ti_Point.size.data));
	Point &p = *dp2.get<Point>();
	p.x = 0x5;
	p.y = 0x7;
	//println(add_int(p, p));

	print_data(dp2, CVM::MemorySize(8));

	tim[TypeIndex(0)] = ti_nil;
	tim[TypeIndex(1)] = ti_int;
	tim[TypeIndex(2)] = ti_Point;

	println("=========");

	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, tim));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(CreateFunction_Main(), tim);

	VM.Genv().addSubEnvironment(lenv);

	VM.Call(*lenv);

	return 0;
}
