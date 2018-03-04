#include "basic.h"
#include "parse.h"
#include "inststruct/instcode.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "bignumber.h"
#include <regex>

namespace CVM
{
	using ParsedIdentifier = PriLib::ExplicitType<std::string>;

	enum ParseErrorCode
	{
		PEC_NumTooLarge,
		PEC_NumIsSigned,
		PEC_URDid,
		PEC_URNum,
		PEC_URIns,
		PEC_URCmd,
		PEC_UREnv,
		PEC_URReg,
		PEC_UREscape,
		PEC_UFType,
		PEC_UFFunc,
		PEC_UMArgs,
		PEC_DUType,
		PEC_DUFunc,
		PEC_DUDataId,
		PEC_NotFuncReg,
	};

	class ParseInfo
	{
	public:
		explicit ParseInfo(TypeInfoMap &tim)
			: tim(tim) {}

		std::map<std::string, InstStruct::FunctionInfo*> functable;
		InstStruct::FunctionInfo *currfunc;
		TypeInfoMap &tim;
		LiteralDataPoolCreater datamap;
		size_t lcount = 0;
		ParsedIdentifier entry;
		ParsedIdentifier currtype;
		int currsection = 0;
		mutable bool haveerror = false;

		bool check() const {
			for (auto &pfunc : functable) {
				for (auto &arg : pfunc.second->arglist) {
					if (arg > pfunc.second->regsize()) {
						putError("Parse Error for '" + std::string(geterrmsg(PEC_UMArgs)) + "' in function '" + pfunc.first +"'.");
						break;
					}
				}
			}
			if (haveerror)
				return false;
			return true;
		}

		void putErrorLine() const {
			fprintf(stderr, "Parse Error in line(%zu).\n", lcount);
			haveerror = true;
		}
		void putErrorLine(ParseErrorCode pec) const {
			fprintf(stderr, "Parse Error for '%s' in line(%zu).\n", geterrmsg(pec), lcount);
			haveerror = true;
		}
		void putErrorLine(ParseErrorCode pec, const std::string &msg) const {
			fprintf(stderr, "Parse Error for '%s' at '%s' in line(%zu).\n", geterrmsg(pec), msg.c_str(), lcount);
			haveerror = true;
		}
		void putError(const std::string &msg) const {
			fprintf(stderr, "%s\n", msg.c_str());
			haveerror = true;
		}

		const char* geterrmsg(ParseErrorCode pec) const {
			static std::map<ParseErrorCode, const char*> pecmap = {
				{ PEC_NumTooLarge, "Number too large" },
				{ PEC_NumIsSigned, "Number is signed" },
				{ PEC_URDid, "Unrecognized data index" },
				{ PEC_URNum, "Unrecognized number" },
				{ PEC_URCmd, "Unrecognized command" },
				{ PEC_URIns, "Unrecognized instruction" },
				{ PEC_UREnv, "Unrecognized environment" },
				{ PEC_URReg, "Unrecognized register" },
				{ PEC_UREscape, "Unrecognized escape" },
				{ PEC_UFType, "Unfind type" },
				{ PEC_UFFunc, "Unfind function" },
				{ PEC_UMArgs, "Unmatch arguments" },
				{ PEC_DUType, "type name duplicate" },
				{ PEC_DUFunc, "func name duplicate" },
				{ PEC_DUDataId, "data index duplicate" },
				{ PEC_NotFuncReg, "not function's register" },
			};
			return pecmap.at(pec);
		}
	};

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim) {
		return PriLib::StorePtr<ParseInfo>(new ParseInfo(tim));
	}

	bool haveError(const ParseInfo &parseinfo) {
		return !parseinfo.check();
	}

	template <typename T>
	T parseNumber(ParseInfo &parseinfo, const std::string &word) {
		if (!std::numeric_limits<T>::is_signed) {
			if (word[0] == '-') {
				parseinfo.putErrorLine(PEC_NumIsSigned, word);
				return 0;
			}
		}
		return PriLib::Convert::to_integer<T>(word, [&]() {
			if (PriLib::Convert::is_integer(word))
				parseinfo.putErrorLine(PEC_NumTooLarge);
			else
				parseinfo.putErrorLine(PEC_URNum);
			});
	}

	template <typename T>
	void parseNumber(ParseInfo &parseinfo, T &result, const std::string &word) {
		result = parseNumber<T>(parseinfo, word);
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word);

	InstStruct::Register parseRegister(ParseInfo &parseinfo, const std::string &word) {
		if (word[0] != '%') {
			parseinfo.putErrorLine(PEC_URReg, word);
			return InstStruct::Register();
		}
		if (word == "%res")
			return InstStruct::Register::ResultRegister();
		else if (word == "%0")
			return InstStruct::Register::ZeroRegister();
		else if (word == "%sp")
			return InstStruct::Register::StackPointerRegister();

		std::smatch sm;
		if (std::regex_match(word, sm, std::regex(R"S(%([tg])?(\d+))S"))) {
			Config::RegisterIndexType index;
			InstStruct::EnvType etype;
			parseNumber(parseinfo, index, sm[2].str());
			etype = InstStruct::e_current;
			switch (sm[1].str()[0]) {
			case '\0':
				return InstStruct::Register::PrivateDataRegister(index, etype);
			case 't':
				return InstStruct::Register::ThreadDataRegister(index);
			case 'g':
				return InstStruct::Register::GlobalDataRegister(index);
			}
		}
		else if (std::regex_match(word, sm, std::regex(R"S(%(\d+)\(\%(\w+)(?:\((.+)\))?\))S"))) {
			auto index = parseNumber<Config::RegisterIndexType>(parseinfo, sm[1].str());
			InstStruct::EnvType etype;
			auto estr = sm[2].str();
			if (estr == "env")
				etype = InstStruct::e_current;
			else if (estr == "tenv")
				etype = InstStruct::e_temp;
			else if (estr == "penv")
				etype = InstStruct::e_parent;
			else {
				parseinfo.putErrorLine(PEC_UREnv);
			}

			if (sm[3].matched) {
				TypeIndex tindex = parseType(parseinfo, sm[3].str());\
				return InstStruct::Register::PrivateDataRegister(index, etype, tindex);
			}
			else {
				return InstStruct::Register::PrivateDataRegister(index, etype);
			}
		}
		else if (std::regex_match(word, sm, std::regex(R"S(%sp\((\d+)\)(!)?)S"))) {
			auto size = parseNumber<Config::MemorySizeType>(parseinfo, sm[1].str());
			if (sm[2].str()[0] == '!')
				return InstStruct::Register::StackRegisterSizeDecrease(MemorySize(size));
			else
				return InstStruct::Register::StackRegisterSize(MemorySize(size), 0);
		}
		else if (std::regex_match(word, sm, std::regex(R"S(%sp\((.+)\)(!)?)S"))) {
			TypeIndex index = parseType(parseinfo, sm[1].str());
			if (sm[2].str()[0] == '!')
				return InstStruct::Register::StackRegisterTypeDecrease(index);
			else
				return InstStruct::Register::StackRegisterType(index, 0);
		}
		else if (std::regex_match(word, sm, std::regex(R"S(%sp\[(\d+)\]\(\d+\))S"))) {
			auto offset = parseNumber<Config::StackOffsetType>(parseinfo, sm[1].str());
			auto size = parseNumber<Config::MemorySizeType>(parseinfo, sm[2].str());
			return InstStruct::Register::StackRegisterSize(MemorySize(size), offset);
		}
		else if (std::regex_match(word, sm, std::regex(R"S(%sp\[(\d+)\]\(.+\))S"))) {
			auto offset = parseNumber<Config::StackOffsetType>(parseinfo, sm[1].str());
			TypeIndex index = parseType(parseinfo, sm[1].str());
			return InstStruct::Register::StackRegisterType(index, offset);
		}
		else {
			parseinfo.putErrorLine(PEC_URReg);
		}

		return InstStruct::Register();
	}

	InstStruct::Data parseDataInst(ParseInfo &parseinfo, const std::string &word) {
		auto errfunc = [&]() {
			parseinfo.putErrorLine(PEC_URNum, word);
			parseinfo.putError("The number must be unsigned integer and below " + std::to_string(8 * sizeof(InstStruct::Data::Type)) + "bits.");
		};

		std::string nword = word;
		int base = 10;

		if (word.length() > 2 && word[0] == '0' && word[1] == 'x') {
			base = 16;
			nword = word.substr(2);
		}

		return InstStruct::Data(PriLib::Convert::to_integer<InstStruct::Data::Type>(nword, errfunc, base));
	}

	InstStruct::DataIndex parseDataIndex(ParseInfo &parseinfo, const std::string &word) {
		if (word.empty() || word[0] != '#')
			parseinfo.putErrorLine(PEC_URDid, word);

		return InstStruct::DataIndex(parseNumber<InstStruct::DataIndex::Type>(parseinfo, word.substr(1)));
	}

	void parseDataLarge(ParseInfo &parseinfo, const std::string &word, uint8_t *buffer, size_t size) {
		BigInteger bi;
		if (bi.parseu(word)) {
			bool is_large;
			if (!bi.toBufferLSB(buffer, size, is_large)) {
				if (is_large) {
					parseinfo.putErrorLine(PEC_NumTooLarge, word);
					parseinfo.putError("The accepted size is " + std::to_string(size) + " byte(s).");
				}
				else {
					parseinfo.putErrorLine(PEC_URNum, word);
				}
			}
		}
		else {
			if (word[0] == '-')
				parseinfo.putErrorLine(PEC_NumIsSigned, word);
			else
				parseinfo.putErrorLine(PEC_URNum, word);
		}
	}
	bool parseDataLarge(ParseInfo &parseinfo, const std::string &word, std::function<uint8_t*(size_t)> creater) {
		BigInteger bi;
		if (bi.parseu(word)) {
			size_t size = bi.size();
			uint8_t *buffer = creater(size);
			if (bi.toBufferLSB(buffer, size)) {
				return true;
			}
			else {
				parseinfo.putErrorLine(PEC_URNum, word);
			}
		}
		else {
			if (word[0] == '-')
				parseinfo.putErrorLine(PEC_NumIsSigned, word);
			else
				parseinfo.putErrorLine(PEC_URNum, word);
		}
		return false;
	}

	ParsedIdentifier parseIdentifier(ParseInfo &parseinfo, const std::string &word) {
		if (!word.empty()) {
			bool escape = false;
			if (word[0] == '%' || word[0] == '#') {
				escape = true;
			}
			if (escape) {
				if (word.size() == 1 || word[1] != word[0])  {
					parseinfo.putErrorLine(PEC_UREscape, word);
				}
				return ParsedIdentifier(word.substr(1));
			}
		}
		return ParsedIdentifier(word);
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
		if (!start)
			f2(parseinfo, code, list);
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word) {
		TypeIndex index;
		if (parseinfo.tim.find(parseIdentifier(parseinfo, word).data, index)) {
			return index;
		}
		else {
			parseinfo.putErrorLine(PEC_UFType, word);
			return TypeIndex(0);
		}
	}
}

namespace CVM
{
	enum KeySection : int {
		ks_nil = 0,
		ks_program,
		ks_imports,
		ks_exports,
		ks_datas,
		ks_module,
		ks_func,
		ks_type,
	};

	const auto& getSectionKeyMap() {
		static std::map<std::string, int> map {
			{ "program", ks_program },
			{ "imports", ks_imports },
			{ "exports", ks_exports },
			{ "datas", ks_datas },
			{ "module", ks_module },
			{ "func", ks_func },
			{ "type", ks_type },
		};
		return map;
	}
	void parseSection(ParseInfo &parseinfo, int code, const std::vector<std::string> &list) {
		switch (code) {
		case ks_func:
		{
			if (list.size() != 1) {
				parseinfo.putErrorLine();
			}
			const auto &name = parseIdentifier(parseinfo, list.at(0));
			if (parseinfo.functable.find(name.data) == parseinfo.functable.end()) {
				auto fp = new InstStruct::FunctionInfo();
				parseinfo.functable[list[0]] = fp;
				parseinfo.currfunc = fp;
			}
			else {
				parseinfo.putErrorLine(PEC_DUFunc);
			}
			break;
		}
		case ks_type:
		{
			if (list.size() != 1) {
				parseinfo.putErrorLine();
			}
			const auto &name = parseIdentifier(parseinfo, list.at(0));
			TypeIndex tid;
			if (parseinfo.tim.find(name.data, tid)) {
				parseinfo.putErrorLine(PEC_DUType);
			}
			else {
				parseinfo.currtype = name;
				parseinfo.tim.insert(name.data, TypeInfo());
			}
			break;
		}
		}
	}

	void parseSectionInside(ParseInfo &parseinfo, const std::string &code, const std::vector<std::string> &list) {
		using ParseInsideProcess = std::function<void(ParseInfo &, const std::vector<std::string> &)>;
		using ParseInsideMap = std::map<std::string, ParseInsideProcess>;
		static std::map<int, ParseInsideMap> parsemap {
			{
				ks_func,
				ParseInsideMap {
					{
						"arg",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.empty()) {
							}
							else {
								if (list[0][0] == '%') {
									for (const auto &word : list) {
										InstStruct::Register reg = parseRegister(parseinfo, word);
										if (reg.isPrivateDataRegister())
											parseinfo.currfunc->arglist.push_back(reg.index());
										else
											parseinfo.putErrorLine(PEC_NotFuncReg, word);
									}
								}
								else {
									if (list.size() == 1) {
										auto count = parseNumber<InstStruct::Register::RegisterIndexType>(parseinfo, list[0]);
										for (InstStruct::Register::RegisterIndexType i = 0; i != count; ++i) {
											parseinfo.currfunc->arglist.push_back(i);
										}
									}
									else {
										parseinfo.putErrorLine();
									}
								}
							}
						}
					},
					{
						"dyvarb",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 1) {
								parseNumber(parseinfo, parseinfo.currfunc->dyvarb_count, list[0]);
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"stvarb",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 2) {
								auto &type = list[list.size() - 1];
								size_t count = parseNumber<size_t>(parseinfo, list[0]);
								TypeIndex index = parseType(parseinfo, type);
								for (size_t i = 0; i < count; ++i)
									parseinfo.currfunc->stvarb_typelist.push_back(index);
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
				}
			},
			{
				ks_program,
				ParseInsideMap {
					{
						"entry",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 1) {
								parseinfo.entry = parseIdentifier(parseinfo, list[0]);
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
				},
			},
			{
				ks_type,
				ParseInsideMap {
					{
						"size",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							const auto &name = parseinfo.currtype;
							auto &typeinfo = parseinfo.tim.at(name.data);
							if (list.size() == 1) {
								parseNumber(parseinfo, typeinfo.size.data, list[0]);
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
				},
			},
			{
				ks_datas,
				ParseInsideMap {
					{
						"data",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 2 || list.size() == 3) {
								InstStruct::DataIndex di = parseDataIndex(parseinfo, list[0]);
								auto iter = parseinfo.datamap.find(di.index());
								if (iter == parseinfo.datamap.end()) {
									uint8_t *buffer = nullptr;
									size_t msize = 0;
									if (list.size() == 3) {
										msize = parseNumber<size_t>(parseinfo, list[2]);
										buffer = new uint8_t[msize]();
										parseDataLarge(parseinfo, list[1], buffer, msize);
									}
									else {
										if (!parseDataLarge(parseinfo, list[1], [&](size_t size) {
											msize = size;
											return buffer = new uint8_t[size]();
										}))
											delete[] buffer;
									}
									parseinfo.datamap[di.index()] = std::make_pair(buffer, static_cast<uint32_t>(msize));
								}
								else {
									parseinfo.putErrorLine(PEC_DUDataId);
								}
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"array",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() >= 2) {
								InstStruct::DataIndex di = parseDataIndex(parseinfo, list[0]);
								auto iter = parseinfo.datamap.find(di.index());
								if (iter == parseinfo.datamap.end()) {
									std::vector<uint8_t> vec;
									for (auto &word : PriLib::rangei(list.begin() + 1, list.end())) {
										BigInteger bi;
										if (bi.parseu(word)) {
											bool is_large;
											uint8_t v;
											if (bi.toBuffer(&v, 1, is_large)) {
												vec.push_back(v);
											}
											else {
												if (is_large) {
													parseinfo.putErrorLine(PEC_NumTooLarge, word);
													parseinfo.putError("Only accept 1 byte for each data.");
													return;
												}
												else {
													parseinfo.putErrorLine(PEC_URNum, word);
													return;
												}
											}
										}
										else {
											if (word[0] == '-')
												parseinfo.putErrorLine(PEC_NumIsSigned, word);
											else
												parseinfo.putErrorLine(PEC_URNum, word);
											return;
										}
									}

									uint8_t *buffer = new uint8_t[vec.size()]();
									PriLib::Memory::copyTo(buffer, vec.data(), vec.size());
									parseinfo.datamap[di.index()] = std::make_pair(buffer, static_cast<uint32_t>(vec.size())); // TODO
								}
								else {
									parseinfo.putErrorLine(PEC_DUDataId);
									return;
								}
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
				},
			},
		};

		auto iter = parsemap.find(parseinfo.currsection);
		if (iter != parsemap.end()) {
			auto iiter = iter->second.find(code);
			if (iiter != iter->second.end()) {
				iiter->second(parseinfo, list);
			}
			else {
				parseinfo.putErrorLine(PEC_URCmd);
			}
		}
		else {
			parseinfo.putErrorLine(PEC_URCmd);
		}
	}

	InstStruct::Instruction* parseFuncInstBase(ParseInfo& parseinfo, const std::string &code, const std::vector<std::string> &list);

	void parseFuncInst(ParseInfo &parseinfo, const std::string &code, const std::vector<std::string> &list)
	{
		return parseinfo.currfunc->instdata.push_back(parseFuncInstBase(parseinfo, code, list));
	}

	void parseLine(ParseInfo &parseinfo, const std::string &line)
	{
		char fc = line[0];

		if (fc == '.') {
			parseLineBase(parseinfo, line,
				[&](const char *code) {
					auto &map = getSectionKeyMap();
					auto iter = map.find(code + 1);
					if (iter != map.end()) {
						parseinfo.currsection = iter->second;
						return iter->second;
					}
					else
						parseinfo.putErrorLine();
					return 0;
				},
				parseSection);
		}
		else if (std::isblank(fc)) {
			std::string cmd;
			parseLineBase(parseinfo, line,
				[&](const char *code) {
					int isinst = code[0] != '.';
					cmd = isinst ? code : code + 1;
					return isinst;
				},
				[&](ParseInfo &parseinfo, int isinst, const std::vector<std::string> &list) {
					(isinst ? parseFuncInst : parseSectionInside)(parseinfo, cmd, list);
				});
		}
		else {
			parseinfo.putErrorLine();
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

	std::string getEntry(ParseInfo &parseinfo) {
		return parseinfo.entry.data;
	}
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo & parseinfo)
	{
		return parseinfo.datamap;
	}
}

#include "inststruct/instdef.h"

namespace CVM
{
	InstStruct::Instruction* parseFuncInstBase(ParseInfo& parseinfo, const std::string &code, const std::vector<std::string> &list)
	{
		using ParseInstProcess = std::function<InstStruct::Instruction*(ParseInfo &, const std::vector<std::string> &)>;
		using ParseInstMap = std::map<std::string, ParseInstProcess>;

		using namespace InstStruct;

		static ParseInstMap parsemap {
			{
				"mov",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
					return new Insts::Move(parseRegister(parseinfo, list[0]), parseRegister(parseinfo, list[1]));
				}
			},
			{
				"load",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) -> InstStruct::Instruction* {
					if (!list[1].empty() && list[1][0] != '#') {
						return new Insts::Load1(
							parseRegister(parseinfo, list[0]),
							parseDataInst(parseinfo, list[1]),
							parseType(parseinfo, list[2]));
					}
					else {
						return new Insts::Load2(
							parseRegister(parseinfo, list[0]),
							parseDataIndex(parseinfo, list[1]),
							parseType(parseinfo, list[2]));
					}
				}
			},
			{
				"ret",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
					return new Insts::Return();
				}
			},
			{
				"db_opreg",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
					return new Insts::Debug_OutputRegister();
				}
			},
		};

		auto iter = parsemap.find(code);
		if (iter != parsemap.end()) {
			return iter->second(parseinfo, list);
		}
		else {
			parseinfo.putErrorLine(PEC_URIns, code);
			return nullptr;
		}
	}
}
