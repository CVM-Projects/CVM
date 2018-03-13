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

void print_string(const char *msg)
{
	std::printf("%s", msg);
}

void print_int64(int64_t v)
{
	println(v);
}

#include "runtime/function.h"

void _print_string(CVM::Runtime::PointerFunction::Result &result, CVM::Runtime::PointerFunction::ArgumentList &arglist)
{
	auto x = arglist[0].get<const char *>();
	print_string(*x);
}

void _print_int64(CVM::Runtime::PointerFunction::Result &result, CVM::Runtime::PointerFunction::ArgumentList &arglist)
{
	auto x = arglist[0].get<int64_t>();
	print_int64(*x);
}

void _int64_add(CVM::Runtime::PointerFunction::Result &result, CVM::Runtime::PointerFunction::ArgumentList &arglist)
{
	auto x = *arglist[0].get<int64_t>();
	auto y = *arglist[1].get<int64_t>();

	*result.get<int64_t>() = x + y;
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

	// Run 'main'

	VirtualMachine VM;
	
	LiteralDataPool ldp(datasmap);
	Runtime::FuncTable functable;

	for (auto &func : fset) {
		if (functable.findKey(func.first) == functable.size()) {
			Runtime::Function *f = new Runtime::InstFunction(Compile::Compile(*func.second));
			functable.insert(func.first, f);
		}
		else {
			assert(false);
		}
	}

	Runtime::Function *func = functable.getValue(functable.findKey(entry));

	functable.insert("print_string", new Runtime::PointerFunction(&_print_string));
	functable.insert("print_int64", new Runtime::PointerFunction(&_print_int64));
	functable.insert("cms#int64#+", new Runtime::PointerFunction(&_int64_add));

	println(ldp.toString());
	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, tim, ldp, functable));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(static_cast<Runtime::InstFunction&>(*func), tim);

	VM.Genv().addSubEnvironment(lenv);

	VM.Call(*lenv);

	return 0;
}
