#include "basic.h"
#include "virtualmachine.h"
#include "typeinfo.h"
#include "compiler/compile.h"
#include "parser/parse.h"
#include "runtime/environment.h"
#include "runtime/datapointer.h"
#include "runtime/datamanage.h"

int add_int(int x, int y) {
	return x + y;
}

struct Point
{
	int x;
	int y;
};

Point add_Point(Point x, Point y)
{
	return Point{ x.x + y.x, x.y + y.y };
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
	void VirtualMachine::Call(Runtime::LocalEnvironment *env) {
		if (this->_currenv)
			this->_currenv->addSubEnvironment(env);
		this->_currenv = env;
	}

	void VirtualMachine::Launch() {
		while (this->_currenv) {
			auto &env = *this->_currenv;
			auto &cflow = env.Controlflow();
			cflow.init();
			cflow.callCurrInst(env);
			cflow.incProgramCounter();

			if (!cflow.isInstRunning()) { // if 'ret'
				if (env.PEnv().isLocal()) {
					auto *oldenv = this->_currenv;
					this->_currenv = &static_cast<Runtime::LocalEnvironment&>(env.PEnv());
					this->_currenv->removeSubEnvironment(oldenv);
				}
				else {
					this->_currenv = nullptr;
					printf("Program Over\n");
				}
			}
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

void print_int64x(int64_t v)
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

void _print_int64x(CVM::Runtime::PointerFunction::Result &result, CVM::Runtime::PointerFunction::ArgumentList &arglist)
{
	auto x = arglist[0].get<int64_t>();
	print_int64x(*x);
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

static const CVM::Runtime::PtrFuncMap& getInsidePtrFuncMap(CVM::HashStringPool &hashStringPool)
{
	static const CVM::Runtime::PtrFuncMap pfm {
		{ hashStringPool.insert("print_string"), _print_string },
		{ hashStringPool.insert("print_int64"), _print_int64 },
		{ hashStringPool.insert("print_int64x"), _print_int64x },
		{ hashStringPool.insert("cms#int64#+"), _int64_add },
		{ hashStringPool.insert("system"), _system }
	};

	return pfm;
}

void pause() {
#if (defined(TEST_MEMORY))
	print("Pause");
	getchar();
#endif
}

#include "inststruct/hashstringpool.h"

CVM::Runtime::LocalEnvironment * createVM(PriLib::TextFile &cmsfile, CVM::VirtualMachine &VM)
{
	// Init GlobalInfo

	CVM::InstStruct::GlobalInfo *globalinfo = new CVM::InstStruct::GlobalInfo();

	// Parse File

	using namespace CVM;

	Compiler compiler;
	Runtime::FuncTable *functable;
	NewLiteralDataPool *ldp;

	{
		// Parse File
		auto parseinfo = createParseInfo(*globalinfo);
		parseFile(parseinfo, cmsfile);
		cmsfile.close();
		pause();

		if (haveError(parseinfo)) {
			exit(-1);
		}

		// Create LiteralDataPool
		NewLiteralDataPool &datasmap = getDataSectionMap(parseinfo);
		ldp = &datasmap;
		println(ldp->toString());

		// Create FuncTable
		functable = new Runtime::FuncTable();

		// Compile
		if (!compiler.compile(parseinfo, getInsidePtrFuncMap(globalinfo->hashStringPool), *functable)) {
			println("Compiled Error.");
			exit(-1);
		}
	}

	VM.addGlobalEnvironment(Compile::CreateGlobalEnvironment(0xff, &globalinfo->typeInfoMap, ldp, functable, &globalinfo->hashStringPool));

	Config::FuncIndexType entry_id = compiler.getEntryID();
	Runtime::InstFunction &entry_func = static_cast<Runtime::InstFunction&>(*functable->at(entry_id));
	Runtime::LocalEnvironment *lenv = Compile::CreateLoaclEnvironment(entry_func, globalinfo->typeInfoMap);

	VM.Genv().addSubEnvironment(lenv);
	pause();

	return lenv;
}

#include "inststruct/info.h"

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

	// Run 'main'

	CVM::VirtualMachine VM;

	CVM::Runtime::LocalEnvironment *lenv = createVM(cmsfile, VM);

	VM.Call(lenv);

	VM.Launch();

	pause();

	return 0;
}
