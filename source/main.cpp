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

void _system(CVM::Runtime::PointerFunction::Result &result, CVM::Runtime::PointerFunction::ArgumentList &arglist)
{
	std::system(*arglist[0].get<const char*>());
}

#include "runtime/function.h"

static const CVM::Runtime::PtrFuncMap& getInsidePtrFuncMap()
{
	static const CVM::Runtime::PtrFuncMap pfm {
		{ "print_string", _print_string },
		{ "print_int64", _print_int64 },
		{ "cms#int64#+", _int64_add },
		{ "system", _system }
	};

	return pfm;
}

void pause() {
	//print("Pause");getchar();
}

CVM::Runtime::LocalEnvironment * createVM(PriLib::TextFile &cmsfile, CVM::VirtualMachine &VM)
{
	// Init TypeInfoMap

	CVM::TypeInfoMap *tim = new CVM::TypeInfoMap();

	// Parse File

	using namespace CVM;

	Compiler compiler;
	Runtime::FuncTable *functable;
	LiteralDataPool *ldp;

	{
		// Parse File
		auto parseinfo = createParseInfo(*tim);
		parseFile(parseinfo, cmsfile);
		cmsfile.close();
		pause();

		// Create LiteralDataPool
		LiteralDataPoolCreater &datasmap = getDataSectionMap(parseinfo);
		ldp = new LiteralDataPool(datasmap);

		// Create FuncTable
		functable = new Runtime::FuncTable();

		// Compile
		if (!compiler.compile(parseinfo, getInsidePtrFuncMap(), *functable)) {
			println("Compiled Error.");
			exit(-1);
		}
	}

	Config::FuncIndexType entry_id = compiler.getEntryID();
	Runtime::Function *entry_func = functable->at(entry_id);

	println(ldp->toString());
	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, tim, ldp, functable));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(static_cast<Runtime::InstFunction&>(*entry_func), *tim);

	VM.Genv().addSubEnvironment(lenv);
	pause();

	return lenv;
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


	using namespace CVM;

	// Run 'main'

	VirtualMachine VM;

	Runtime::LocalEnvironment *lenv = createVM(cmsfile, VM);

	VM.Call(*lenv);

	pause();

	return 0;
}
