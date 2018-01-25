#include "basic.h"
#include "parse.h"
#include "inststruct/instcode.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/instdef.h"
#include <regex>

namespace CVM
{
	class ParseInfo
	{
	public:
		explicit ParseInfo(TypeInfoMap &tim)
			: tim(tim) {}

		std::map<std::string, InstStruct::FunctionInfo*> functable;
		InstStruct::FunctionInfo *currfunc;
		TypeInfoMap &tim;
		size_t lcount = 0;
	};

	enum KeySection : int {
		ks_program,
		ks_imports,
		ks_exports,
		ks_datas,
		ks_module,
		ks_func,
	};

	enum KeyInside : int {
		ki_mode,
		ki_arg,
		ki_dyvarb,
		ki_stvarb,
		ki_data,
		ki_string,
		ki_func,
		ki_entry,
	};

	using ParseKeyMap = std::map<std::string, int>;

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim) {
		return PriLib::StorePtr<ParseInfo>(new ParseInfo(tim));
	}

	void putErrorLine(size_t lcount) {
		fprintf(stderr, "Parse Error in line(%s).\n", std::to_string(lcount).c_str());
	}
	void putErrorLine(const std::string &msg, size_t lcount) {
		fprintf(stderr, "Parse Error for '%s' in line(%s).\n", msg.c_str(), std::to_string(lcount).c_str());
	}

	InstStruct::Register parseRegister(ParseInfo &parseinfo, const std::string &word) {
		if (word[0] != '%') {
			putErrorLine(parseinfo.lcount);
			return InstStruct::Register();
		}
		if (word == "%res")
			return InstStruct::Register(InstStruct::r_res);
		else if (word == "%0")
			return InstStruct::Register(InstStruct::r_0);

		InstStruct::RegisterType rtype;
		InstStruct::EnvType etype;
		uint16_t index;

		const char *mword = word.c_str() + 1;

		switch (word[1]) {
		case 'g':
			rtype = InstStruct::r_g;
			mword++;
			break;
		case 't':
			rtype = InstStruct::r_t;
			mword++;
			break;
		default:
			if (std::isdigit(word[1])) {
				rtype = InstStruct::r_n;
				break;
			}
			else {
				putErrorLine(parseinfo.lcount);
				break;
			}
		}


		std::regex rc("(\\d+)");
		std::regex re("(\\d+)\\(\\%(\\w+)\\)");

		std::cmatch cm;
		if (std::regex_match(mword, cm, rc)) {
			index = PriLib::Convert::to_integer<uint16_t>(cm[1].str(), [&]() { putErrorLine("Number to large", parseinfo.lcount); });
			etype = InstStruct::e_current;
		}
		else if ((std::regex_match(mword, cm, re))) {
			index = PriLib::Convert::to_integer<uint16_t>(cm[1].str(), [&]() { putErrorLine("Number to large", parseinfo.lcount); });
			auto estr = cm[2].str();
			if (estr == "env") {
				etype = InstStruct::e_current;
			}
			else if (estr == "tenv") {
				etype = InstStruct::e_temp;
			}
			else if (estr == "penv") {
				etype = InstStruct::e_parent;
			}
			else {
				putErrorLine("Unrecognized environment", parseinfo.lcount);
			}
		}
		else {
			putErrorLine("Unrecognized register", parseinfo.lcount);
		}

		return InstStruct::Register(rtype, etype, index);
	}

	std::string parseIdentifier(ParseInfo &parseinfo, const std::string &word) {
		std::string mword;
		bool escape = false; // Use '%' in identifier.
		for (auto &c : word) {
			if (escape) {
				escape = false;
				if (c == '%' || c == '#')
					mword.push_back(c);
				else
					putErrorLine("Unrecognized escape", parseinfo.lcount);
			}
			else {
				if (c == '%')
					escape = true;
				else
					mword.push_back(c);
			}
		}
		if (escape) {
			putErrorLine("Unrecognized escape", parseinfo.lcount);
		}
		PriLib::Convert::split(mword, "#", [&](const char *w) {
			//println(w);
			});
		return mword;
	}

	void parseLineBase(
		ParseInfo &parseinfo,
		const std::string &line,
		std::function<int(const char*)> f1,
		std::function<void(ParseInfo&, int, const std::vector<std::string>&)> f2)
	{
		const char *blanks = " \t,";
		bool start = true;
		int code = 0;
		std::vector<std::string> list;
		PriLib::Convert::split(line, blanks, [&](const char *s) {
			if (start) {
				code = f1(s);
				start = false;
			}
			else {
				list.push_back(s);
			}
			});
		f2(parseinfo, code, list);
	}

	static const ParseKeyMap& GetParseKeyInsideMap() {
		static ParseKeyMap map {
			{ "func", ki_func },
			{ "mode", ki_mode },
			// func
			{ "arg", ki_arg },
			{ "dyvarb", ki_dyvarb },
			{ "stvarb", ki_stvarb },
			// datas
			{ "data", ki_data },
			{ "string", ki_string },
			// program
			{ "entry", ki_entry },
		};
		return map;
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word) {
		TypeIndex index;
		if (parseinfo.tim.find(parseIdentifier(parseinfo, word), index)) {
			return index;
		}
		else {
			putErrorLine("Unfind type", parseinfo.lcount);
			return TypeIndex(0);
		}
	}

	void parseFuncInside(ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
		// KeyInside
		switch (code) {
		case ki_arg:
			//parseinfo.currfunc->arglist
			break;
		case ki_dyvarb:
			if (list.size() == 1) {
				parseinfo.currfunc->dyvarb_count = PriLib::Convert::to_integer<size_t>(list[0], [&]() { putErrorLine("Error Num", parseinfo.lcount); });
			}
			else {
				putErrorLine(parseinfo.lcount);
			}
			break;
		case ki_stvarb:
			if (list.size() == 2) {
				auto &type = list[list.size() - 1];
				size_t count = PriLib::Convert::to_integer<size_t>(list[0], [&]() { putErrorLine("Error Num", parseinfo.lcount); });
				TypeIndex index = parseType(parseinfo, type);
				for (size_t i = 0; i < count; ++i)
					parseinfo.currfunc->stvarb_typelist.push_back(index);
			}
			else {
				putErrorLine(parseinfo.lcount);
			}
			break;
		case ki_data:
			break;
		}
	}

	static const ParseKeyMap& GetPaseKeyInstMap() {
		// TODO
		static ParseKeyMap map {
			{ "mov", InstStruct::InstCode::i_mov },
			{ "load", InstStruct::InstCode::i_load },
			{ "db_opreg", InstStruct::InstCode::id_opreg },
			{ "ret", InstStruct::InstCode::i_ret },
		};
		return map;
	}

	void parseFuncInst(ParseInfo &parseinfo, int code, const std::vector<std::string> &list)
	{
		using namespace InstStruct;
		auto &func = parseinfo.currfunc;
		switch (code) {
		case i_mov:
			//PriLib::Output::println("Move");
			func->instdata.push_back(new Insts::Move(parseRegister(parseinfo, list[0]), parseRegister(parseinfo, list[1])));
			break;
		case i_load:
			//PriLib::Output::println("Load");
			func->instdata.push_back(new Insts::Load1(parseRegister(parseinfo, list[0]), Data(PriLib::Convert::to_integer<uint32_t>(list[1], [&]() { putErrorLine("Error Num", parseinfo.lcount); })), parseType(parseinfo, list[2])));
			break;
		case id_opreg:
			//PriLib::Output::println("Debug_OutputRegister");
			func->instdata.push_back(new Insts::Debug_OutputRegister());
			break;
		case i_ret:
			//PriLib::Output::println("Return");
			func->instdata.push_back(new Insts::Return());
			break;
		}
	}

	void parseLine(ParseInfo &parseinfo, const std::string &line)
	{
		char fc = line[0];

		if (fc == '.') {
			parseLineBase(parseinfo, line,
				[&](const char *code) {
					static ParseKeyMap map {
						{ "program", ks_program },
						{ "imports", ks_imports },
						{ "exports", ks_exports },
						{ "datas", ks_datas },
						{ "module", ks_module },
						{ "func", ks_func },
					};
					auto iter = map.find(code + 1);
					if (iter != map.end())
						return iter->second;
					else
						putErrorLine(parseinfo.lcount);
					return 0;
				},
				[&](ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
					switch (code) {
					case ks_func:
						if (list.size() != 1) {
							putErrorLine(parseinfo.lcount);
						}
						const auto &name = parseIdentifier(parseinfo, list.at(0));
						if (parseinfo.functable.find(name) == parseinfo.functable.end()) {
							auto fp = new InstStruct::FunctionInfo();
							parseinfo.functable[list[0]] = fp;
							parseinfo.currfunc = fp;
						}
						else {
							putErrorLine("func name duplicate", parseinfo.lcount);
						}
						break;
					}
				});
		}
		else if (std::isblank(fc)) {
			bool isinst;
			parseLineBase(parseinfo, line,
				[&](const char *code) {
					isinst = code[0] != '.';
					auto &map = isinst ? GetPaseKeyInstMap() : GetParseKeyInsideMap();
					auto iter = map.find(code + (isinst ? 0 : 1));
					if (iter != map.end())
						return iter->second;
					else
						putErrorLine(parseinfo.lcount);
					return 0;
				},
				[&](ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
					(isinst ? parseFuncInst : parseFuncInside)(parseinfo, code, list);
				});
		}
		else {
			putErrorLine(parseinfo.lcount);
		}
	}

	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file)
	{
		while (!file.eof()) {
			++parseinfo.lcount;

			std::string line = file.getline();

			//PriLib::Output::println("\n(", parseinfo.lcount, ") ", line);

			// Remove Comment
			size_t comment_post = line.find(';');
			if (comment_post != std::string::npos) {
				line = line.substr(0, comment_post);
			}
			if (line.empty())
				continue;

			// Parse Line
			parseLine(parseinfo, line);
		}
	}

	FunctionSet createFunctionSet(ParseInfo &parseinfo) {
		FunctionSet fset;
		for (auto &val : parseinfo.functable) {
			fset[val.first] = new InstStruct::Function(std::move(*parseinfo.functable.at(val.first)));
		}
		return fset;
	}
}
