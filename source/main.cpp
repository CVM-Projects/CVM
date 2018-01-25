#include "basic.h"
#include "virtualmachine.h"
#include "typeinfo.h"
#include "typeinfo.h"
#include "compile.h"
#include "parse.h"
#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include "runtime/environment.h"
#include "runtime/datapointer.h"
#include "runtime/datamanage.h"

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

CVM::TypeInfoMap InitTypeInfoMap()
{
	using namespace CVM;

	TypeInfoMap tim;

	TypeInfo ti_nil;

	ti_nil.index.data = 0;
	ti_nil.name.data = "nil";
	ti_nil.size.data = 0;

	TypeInfo ti_int;

	ti_int.size.data = sizeof(int);

	DataPointer dp1(PriLib::Memory::alloc(ti_int.size.data));
	int &i = *dp1.get<int>();
	i = 10;
	//println(add_int(i, i));

	TypeInfo ti_Point;

	ti_Point.size.data = sizeof(Point);

	DataPointer dp2(PriLib::Memory::alloc(ti_Point.size.data));
	Point &p = *dp2.get<Point>();
	p.x = 0x5;
	p.y = 0x7;

	//print_data(dp2, CVM::MemorySize(8));

	tim.insert("nil", TypeInfo());
	tim.insert("int", ti_int);
	tim.insert("Point", ti_Point);

	return tim;
}
#include <map>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		println("No file to open.");
		return 0;
	}

	// Open File

	PriLib::TextFile cmsfile;

	cmsfile.open(argv[1], PriLib::File::Read);

	if (cmsfile.bad()) {
		putError("Error in open file.");
	}

	// Init TypeInfoMap

	auto tim = InitTypeInfoMap();

	// Parse File

	using namespace CVM;

	auto parseInfo = createParseInfo(tim);
	parseFile(parseInfo, cmsfile);

	// Get func 'main'

	FunctionSet fset = createFunctionSet(parseInfo);
	if (fset.find("main") == fset.end())
	{
		println("Not find 'main' function.");
		return -1;
	}

	InstStruct::Function *func = fset.at("main");

	// Run 'main'

	VirtualMachine VM;

	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, tim));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(*func, tim);

	VM.Genv().addSubEnvironment(lenv);

	VM.Call(*lenv);

	return 0;
}
