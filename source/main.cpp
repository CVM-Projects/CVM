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

	CVM::TypeInfoMap tim;

	// Parse File

	using namespace CVM;

	auto parseInfo = createParseInfo(tim);
	parseFile(parseInfo, cmsfile);

	if (haveError(parseInfo)) {
		return 1;
	}

	auto datasmap = getDataSectionMap(parseInfo);

	// Get entry func

	auto entry = getEntry(parseInfo);
	if (entry.empty()) {
		println("Undeclared entry function.");
		return -1;
	}
	FunctionSet fset = createFunctionSet(parseInfo);
	if (fset.find(entry) == fset.end())
	{
		println("Not find '" + entry + "' function.");
		return -1;
	}

	InstStruct::Function *func = fset.at(entry);

	// Run 'main'

	VirtualMachine VM;
	
	LiteralDataPool ldp(datasmap);
	println(ldp.toString());
	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, tim, ldp));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(*func, tim);

	VM.Genv().addSubEnvironment(lenv);

	VM.Call(*lenv);

	return 0;
}
