#include "parse.h"
#include "file.h"
#include "prints.h"
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <cassert>

#include "inststruct/instcode.h"
#include "inststruct/function.h"

namespace CVM
{
	class ParseInfo
	{
	public:
		std::map<std::string, InstStruct::FunctionInfo*> functable;
		InstStruct::FunctionInfo *currfunc;
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

	static const ParseKeyMap& GetParseKeySectionMap() {
		static ParseKeyMap map {
			{ "program", ks_program },
			{ "imports", ks_imports },
			{ "exports", ks_exports },
			{ "datas", ks_datas },
			{ "module", ks_module },
			{ "func", ks_func },
		};
		return map;
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

	PriLib::StorePtr<ParseInfo> createParseInfo() {
		return PriLib::StorePtr<ParseInfo>(new ParseInfo());
	}

	bool parseRegister(const std::string &word) {
		assert(word[0] == '%');
		return true;
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

	void putErrorLine(size_t lcount) {
		fprintf(stderr, "Parse Error in line(%s).\n", std::to_string(lcount).c_str());
	}
	void putErrorLine(const std::string &msg, size_t lcount) {
		fprintf(stderr, "Parse Error for '%s' in line(%s).\n", msg.c_str(), std::to_string(lcount).c_str());
	}

	void parseLine(ParseInfo &parseinfo, const std::string &line, size_t &lcount)
	{
		char fc = line[0];

		if (fc == '.') {
			parseLineBase(parseinfo, line,
				[&](const char *code) {
					auto &map = GetParseKeySectionMap();
					auto iter = map.find(code + 1);
					if (iter != map.end()) {
						return iter->second;
					}
					else {
						putErrorLine(lcount);
					}
					return 0;
				},
				[&](ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
					switch (code) {
					case ks_func:
						if (list.size() != 1) {
							putErrorLine(lcount);
						}
						auto &name = list.at(0);
						if (parseinfo.functable.find(name) == parseinfo.functable.end()) {
							auto fp = new InstStruct::FunctionInfo();
							parseinfo.functable[list[0]] = fp;
							parseinfo.currfunc = fp;
						}
						else {
							putErrorLine("func name duplicate", lcount);
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
					if (iter != map.end()) {
						return iter->second;
					}
					else {
						putErrorLine(lcount);
					}
					return 0;
				},
				[&](ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
					if (!isinst) {
						// KeyInside
						switch (code) {
						case ki_arg:
							//parseinfo.currfunc->arglist
							break;
						case ki_data:
							break;
						case ki_dyvarb:
							//parseinfo.currfunc->dyvarb_count = PriLib::Convert::
							break;
						}
					}
					else {
						using namespace InstStruct;
						switch (code) {
						case i_mov:
							PriLib::Output::println("Move");
							break;
						case i_load:
							PriLib::Output::println("Load");
							break;
						case id_opreg:
							PriLib::Output::println("Debug_OutputRegister");
							break;
						case i_ret:
							PriLib::Output::println("Return");
						}
					}
				});
		}
		else {
			putErrorLine(lcount);
		}
	}

	void parseFile(ParseInfo &parseinfo, PriLib::TextFile &file)
	{
		size_t lcount = 0;
		while (!file.eof()) {
			++lcount;

			std::string line = file.getline();

			PriLib::Output::println("\n(", lcount, ") ", line);

			// Remove Comment
			size_t comment_post = line.find(';');
			if (comment_post != std::string::npos) {
				line = line.substr(0, comment_post);
			}
			if (line.empty())
				continue;

			// Parse Line
			parseLine(parseinfo, line, lcount);
		}
	}
}
