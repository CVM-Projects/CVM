#include "basic.h"
#include "parse.h"
#include "inststruct/instcode.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/identkeytable.h"
#include "bignumber.h"
#include "inststruct/info.h"
#include "inststruct/register.h"
#include <regex>

namespace CVM
{
	using ParsedIdentifier = HashID;

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
		PEC_URLabel,
		PEC_UREscape,
		PEC_UFType,
		PEC_UFFunc,
		PEC_UMArgs,
		PEC_DUType,
		PEC_DUFunc,
		PEC_DUDataId,
		PEC_NotFuncReg,
		PEC_IllegalFormat,
		PEC_ModeNotAllow,
	};

	class ParseInfo
	{
	public:
		explicit ParseInfo(TypeInfoMap &tim)
			: tim(tim), currfunc_creater(*this) {}

		~ParseInfo() {
			//println("~ParseInfo();");
		}

		class FunctionInfoCreater
		{
		public:
			class LabelKeyTable
			{
			public:
				size_t operator[](const std::string &key) {
					auto iter = _data.find(key);
					if (iter == _data.end()) {
						size_t id = _data.size();
						_data.insert({ key, id });
						_linedata.push_back(std::numeric_limits<Config::LineCountType>::max());
						return id;
					}
					else {
						return iter->second;
					}
				}
				void setLine(size_t id, Config::LineCountType value) {
					Config::LineCountType &line = _linedata[id];
					assert(!hasValue(id));
					line = value;
				}
				Config::LineCountType getLine(size_t id) {
					Config::LineCountType line = _linedata[id];
					assert(hasValue(id));
					return line;
				}
				bool hasValue(size_t id) {
					Config::LineCountType line = _linedata[id];
					return line != std::numeric_limits<Config::LineCountType>::max();
				}

				void clear() {
					_data.clear();
					_linedata.clear();
				}

			private:
				std::map<std::string, size_t> _data;
				std::vector<Config::LineCountType> _linedata;
			};

		public:
			FunctionInfoCreater(ParseInfo &parseinfo)
				: parseinfo(parseinfo) {}

			void set(InstStruct::Function *fip) {
				over();
				currfunc = fip;
				current_line = 0;
			}
			auto get() {
				return currfunc;
			}
			void over() {
				if (currfunc) {
					create_funcinfo();
					adjust_linelabel();
					clear();
				}
			}

		public:
			Config::LineCountType current_line = 0;
			std::vector<FunctionInfoAccesser::ArgumentTypeType> arglist;
			std::vector<FunctionInfoAccesser::StvarbTypeType> sttypelist;
			Config::RegisterIndexType dyvarb_count = 0;
			LabelKeyTable labelkeytable;
			std::list<Config::LineCountType*> rec_line;
			ParseInfo &parseinfo;
			TypeIndex restype;

		private:
			InstStruct::Function *currfunc = nullptr;

			void create_funcinfo() {
				assert(sttypelist.size() <= std::numeric_limits<Config::RegisterIndexType>::max());
				assert(arglist.size() <= std::numeric_limits<Config::RegisterIndexType>::max());
				auto sttypelist_size = static_cast<Config::RegisterIndexType>(sttypelist.size());
				auto arglist_size = static_cast<Config::RegisterIndexType>(arglist.size());
				auto size = FunctionInfoAccesser::GetSize(sttypelist_size, arglist_size);

				currfunc->info.data = FunctionInfo::Type(size);
				FunctionInfoAccesser accesser = currfunc->info.get_accesser();

				accesser.dyvarb_count() = dyvarb_count;
				accesser.stvarb_count() = sttypelist_size;
				accesser.argument_count() = arglist_size;
				accesser.result_type() = restype;
				PriLib::Memory::copyTo(accesser.arglist(), arglist.data(), arglist.size());
				PriLib::Memory::copyTo(accesser.sttypelist(), sttypelist.data(), sttypelist.size());
			}
			void adjust_linelabel() {
				for (Config::LineCountType* p : rec_line) {
					if (labelkeytable.hasValue(*p)) {
						*p = labelkeytable.getLine(*p);
					}
					else {
						parseinfo.putErrorLine();
					}
				}
			}
			void clear() {
				currfunc = nullptr;
				arglist.clear();
				sttypelist.clear();
				dyvarb_count = 0;
				current_line = 0;
				restype = TypeIndex();
				labelkeytable.clear();
				rec_line.clear();
			}

		} currfunc_creater;

		ParsedIdentifier addParsedIdentifier(const std::string &value) {
			return info.hashStringPool.insert(value);
		}

		const std::string& getFromHashStringPool(HashID id) {
			return info.hashStringPool.get(id);
		}

		InstStruct::GlobalInfo info;
		InstStruct::IdentKeyTable functable;
		TypeInfoMap &tim;
		LiteralDataPoolCreater datamap;
		size_t lcount = 0;
		ParsedIdentifier currtype;
		int currsection = 0;
		mutable bool haveerror = false;

		bool check() const {
			/*for (auto &pfunc : functable) {
				for (auto &arg : pfunc.second->arglist) {
					if (arg > pfunc.second->regsize()) {
						putError("Parse Error for '" + std::string(geterrmsg(PEC_UMArgs)) + "' in function '" + pfunc.first + "'.");
						break;
					}
				}
			}*/
			if (haveerror)
				return false;
			return true;
		}

		void putErrorLine() const {
			_putError("Parse Error in line(%zu).\n", lcount);
		}
		void putErrorLine(ParseErrorCode pec) const {
			_putError("Parse Error for '%s' in line(%zu).\n", geterrmsg(pec), lcount);
		}
		void putErrorLine(ParseErrorCode pec, const std::string &msg) const {
			_putError("Parse Error for '%s' at '%s' in line(%zu).\n", geterrmsg(pec), msg.c_str(), lcount);
		}
		void putError(const std::string &msg) const {
			_putError("%s\n", msg.c_str());
		}

	private:
		template <typename... Args>
		void _putError(const char *format, Args... args) const {
			fprintf(stderr, format, args...);
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
				{ PEC_URLabel, "Unrecognized label" },
				{ PEC_UREscape, "Unrecognized escape" },
				{ PEC_UFType, "Unfind type" },
				{ PEC_UFFunc, "Unfind function" },
				{ PEC_UMArgs, "Unmatch arguments" },
				{ PEC_DUType, "type name duplicate" },
				{ PEC_DUFunc, "func name duplicate" },
				{ PEC_DUDataId, "data index duplicate" },
				{ PEC_NotFuncReg, "not function's register" },
				{ PEC_IllegalFormat, "Illegal format" },
				{ PEC_ModeNotAllow, "Mode not allow" }
			};
			return pecmap.at(pec);
		}
	};

	const std::string& getFromHashStringPool(ParseInfo &parseinfo, HashID id) {
		return parseinfo.getFromHashStringPool(id);
	}

	PriLib::StorePtr<ParseInfo> createParseInfo(TypeInfoMap &tim) {
		return PriLib::StorePtr<ParseInfo>(new ParseInfo(tim));
	}

	ParsedIdentifier getEntry(ParseInfo &parseinfo) {
		return parseinfo.info.entry;
	}
	LiteralDataPoolCreater& getDataSectionMap(ParseInfo &parseinfo) {
		return parseinfo.datamap;
	}
	InstStruct::IdentKeyTable& getFunctionTable(ParseInfo &parseinfo) {
		return parseinfo.functable;
	}

	bool haveError(const ParseInfo &parseinfo) {
		return !parseinfo.check();
	}

	InstStruct::GlobalInfo& getGlobalInfo(ParseInfo &parseinfo) {
	    return parseinfo.info;
	}

	bool isEndChar(ParseInfo &parseinfo, char c) {
	    return c == '\0' || std::isspace(c) || c == ',';
	}

	bool isIdentifierChar(ParseInfo &parseinfo, char c) {
        static std::string identcharset("!#$%&*+-./:<=>?@^_~");
        return isalnum(c) || (identcharset.find(c) != identcharset.npos);
	}

	bool hasIdentifierPrefix(ParseInfo &parseinfo, const char *str) {
	    char c = *str;
	    if (c == '\0')
	        return false;
	    if (c == '%' || c == '#')
	        return str[1] == c;
	    return isIdentifierChar(parseinfo, c);
	}

	bool matchPrefix(ParseUnit &parseunit, const PriLib::StringView &substr) {
		PriLib::StringView::OffsetType offset = 0;
		while (parseunit.currview[offset] && substr[offset]) {
			if (parseunit.currview[offset] != substr[offset])
				return false;
			++offset;
		}
		parseunit.currview += offset;
		return true;
	}

	bool matchPrefix(ParseUnit &parseunit, char subchar) {
		if (parseunit.currview[0] != subchar)
			return false;
		parseunit.currview += 1;
		return true;
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
		ParseUnit parseunit(parseinfo, word);
		auto result = InstStruct::Register::Parse(parseunit);
		if (result) {
//			println(word, ":", PriLib::toSubString(parseunit.raw, parseunit.currview), ":", result.value().ToString(getGlobalInfo(parseinfo)));
			return result.value();
		} else {
			parseinfo.putErrorLine(PEC_URReg, word);
			return InstStruct::Register();
		}
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
				if (word.size() == 1 || word[1] != word[0]) {
					parseinfo.putErrorLine(PEC_UREscape, word);
				}
				return parseinfo.addParsedIdentifier(word.substr(1));
			}
		}
		return parseinfo.addParsedIdentifier(word);
	}

	ParsedIdentifier parseLabelKey(ParseInfo &parseinfo, const std::string &word) {
		if (word.size() <= 1 || word[0] != '#' || word[1] == '#' || std::isdigit(word[1])) {
			parseinfo.putErrorLine(PEC_URLabel, word);
		}
		return parseinfo.addParsedIdentifier(word.substr(1));
	}

	static std::pair<size_t, size_t> get_substring(const std::string &line, size_t offset = 0)
	{
		size_t i = line.find('"', offset);
		size_t j = i + 1;
		if (i == line.npos) {
			return std::make_pair(0, 0);
		}

		bool find_enchar = false;
		bool escape = false;
		for (; j < line.size(); j++) {
			if (escape) {
				escape = false;
			}
			else {
				if (line[j] == '\\') {
					escape = true;
				}
				else if (line[j] == '"') {
					find_enchar = true;
					break;
				}
			}
		}

		if (!find_enchar) {
			return std::make_pair(0, 0);
		}

		return std::make_pair(i, j);
	}

	static bool parse_string_escape(const std::string &word, std::string &result) {
		const static auto get_xchar = [](const char *w, const int mode) {
			int c = 0;
			int count = 0;
			const char *p = w;
			while (*p++) count++;
			int i = 1;
			while (count--) {
				char cc = w[count];
				if (mode == 0x10) {
					c += (isdigit(cc) ? (cc - '0') : (isupper(cc) ? (cc - 'A' + 10) : (cc - 'a' + 10))) * i;
					i *= 0x10;
				}
				else if (mode == 010) {
					c += (cc - '0') * i;
					i *= 010;
				}
			}

			return c;
		};
		const static auto get_escape_char = [](char c) {
			switch (c) {
			case '\'': return '\'';
			case '"':  return '"';
			case '?':  return '?';
			case '\\': return '\\';
			case 'a':  return '\a';
			case 'b':  return '\b';
			case 'f':  return '\f';
			case 'n':  return '\n';
			case 'r':  return '\r';
			case 't':  return '\t';
			case 'v':  return '\v';
			default:   return '\0';
			}
		};

		std::string nword;
		char num[4] = "";
		int num_i = 0;
		int mode = 0;
		for (size_t i = 0; i < word.size(); ++i) {
			char c = word[i];
			if (mode == 0) {
				if (c == '\\') {
					mode = 1;
				}
				else {
					nword.push_back(c);
				}
			}
			else if (mode == 1) {
				if (c >= '0' && c <= '7') {
					mode = 2;
					i--;
				}
				else if (c == 'x') {
					mode = 3;
				}
				else if ((c = get_escape_char(c)) != '\0') {
					nword.push_back(c);
					mode = 0;
				}
				else {
					return false;
				}
			}
			else if (mode == 2 || mode == 3) {
				if ((mode == 2 && (c >= '0' && c <= '7')) || (mode == 3 && isxdigit(c))) {
					num[num_i++] = c;
					if (num_i == ((mode == 2) ? 3 : 2)) {
						mode = 0;
					}
				}
				else {
					mode = 0;
					i--;
				}
				if (mode == 0) {
					if (num_i == 0) {
						return false;
					}
					else {
						char cc = get_xchar(num, ((mode == 2) ? 010 : 0x10));
						nword.push_back(cc);
						num[0] = num[1] = num[2] = '\0';
						num_i = 0;
					}
				}
			}
		}
		if (mode == 1) {
			return false;
		}
		else if (mode == 2) {
			nword.push_back(get_xchar(num, 010));
		}
		else if (mode == 3) {
			nword.push_back(get_xchar(num, 0x10));
		}

		result = nword;

		return true;
	}

	void parseLineBase(
		ParseInfo &parseinfo,
		const std::string &xline,
		std::function<int(const char*)> f1,
		std::function<void(ParseInfo&, int, const std::vector<std::string>&)> f2)
	{
		std::string line = xline;

		const char *blanks = " \t,";
		std::vector<std::string> list;

		// Match '"'

		if (line.find('"') != line.npos) {
			std::vector<size_t> rec;

			std::pair<size_t, size_t> p;
			size_t offset = 0;

			do {
				p = get_substring(line, offset);

				if (p.second == 0) {
					break;
				}

				rec.push_back(p.first);
				rec.push_back(p.second);

				offset = p.second + 1;

			} while (true);

			rec.push_back(line.size());

			bool is_string = false;
			size_t reci = 0;
			for (auto &v : rec) {
				std::string xline;
				if (is_string) {
					xline = line.substr(reci, v - reci + 1);
					reci = v + 1;
					list.push_back(xline);
				}
				else {
					xline = line.substr(reci, v - reci);
					reci = v;
					PriLib::Convert::split(xline, blanks, [&](const char *s) { if (*s) list.push_back(s); });
				}
				is_string = !is_string;
			}
		}
		else {
			PriLib::Convert::split(line, blanks, [&](const char *s) { if (*s) list.push_back(s); });
		}

		//

		if (!list.empty()) {
			int code = f1(list[0].c_str());
			f2(parseinfo, code, std::vector<std::string>(list.begin() + 1, list.end()));
		}
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word) {
		TypeIndex index;
		if (parseinfo.tim.find(parseIdentifier(parseinfo, word), index)) {
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
			const auto &namekey = parseIdentifier(parseinfo, list.at(0));
			auto &data = parseinfo.functable.getData(namekey);
			if (data == nullptr) {
				data.reset(new InstStruct::Function());
				parseinfo.currfunc_creater.set(data.get());
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
			const auto &nameid = parseIdentifier(parseinfo, list.at(0));
			TypeIndex tid;
			if (parseinfo.tim.find(nameid, tid)) {
				parseinfo.putErrorLine(PEC_DUType);
			}
			else {
				parseinfo.currtype = nameid;
				parseinfo.tim.insert(nameid, TypeInfo());
			}
			break;
		}
		}
	}

	static uint8_t* createMemory(size_t size) {
		return new uint8_t[size]();
	}

	void parseSectionInside(ParseInfo &parseinfo, const std::string &code, const std::vector<std::string> &list) {
		using ParseInsideProcess = std::function<void(ParseInfo &, const std::vector<std::string> &)>;
		using ParseInsideMap = std::map<std::string, ParseInsideProcess>;
		const static std::map<int, ParseInsideMap> parsemap {
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
											parseinfo.currfunc_creater.arglist.push_back(reg.index());
										else
											parseinfo.putErrorLine(PEC_NotFuncReg, word);
									}
								}
								else {
									if (list.size() == 1) {
										auto count = parseNumber<Config::RegisterIndexType>(parseinfo, list[0]);
										if (parseinfo.info.dataRegisterMode == InstStruct::drm_multiply) {
											if (count != 0) {
												parseinfo.putErrorLine(PEC_ModeNotAllow, list[0]);
												parseinfo.putError("Note: multiply mode not allow this format for '.arg'.");
											}
										}
										else {
											for (Config::RegisterIndexType i = 0; i != count; ++i) {
												parseinfo.currfunc_creater.arglist.push_back(i + 1);
											}
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
						"res",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 1) {
								TypeIndex index = parseType(parseinfo, list[0]);
								parseinfo.currfunc_creater.restype = index;
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"dyvarb",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 1) {
								parseNumber(parseinfo, parseinfo.currfunc_creater.dyvarb_count, list[0]);
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
									parseinfo.currfunc_creater.sttypelist.push_back(index);
							}
							else {
								parseinfo.putErrorLine(PEC_IllegalFormat, "stvarb");
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
								parseinfo.info.entry = parseIdentifier(parseinfo, list[0]);
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"mode",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 1) {
								if (list[0] == "multiply") {
								    parseinfo.info.dataRegisterMode = InstStruct::drm_multiply;
								}
								else if (list[0] == "dynamic") {
								    parseinfo.info.dataRegisterMode = InstStruct::drm_dynamic;
								}
								else if (list[0] == "static") {
								    parseinfo.info.dataRegisterMode = InstStruct::drm_static;
								}
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
							const auto &nameid = parseinfo.currtype;
							auto &typeinfo = parseinfo.tim.at(nameid);
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
										buffer = createMemory(msize);
										parseDataLarge(parseinfo, list[1], buffer, msize);
									}
									else {
										if (!parseDataLarge(parseinfo, list[1], [&](size_t size) {
											msize = size;
											return buffer = createMemory(size);
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

									uint8_t *buffer = createMemory(vec.size());
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
					{
						"string",
						[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
							if (list.size() == 2) {
								InstStruct::DataIndex di = parseDataIndex(parseinfo, list[0]);
								auto iter = parseinfo.datamap.find(di.index());
								if (iter == parseinfo.datamap.end()) {
									std::string nword = list[1].substr(1, list[1].size() - 2);

									if (parse_string_escape(nword, nword)) {
										size_t msize = nword.size() + 1;
										uint8_t *buffer = createMemory(msize);
										for (size_t i = 0; i < nword.size(); ++i) {
											buffer[i] = (uint8_t)nword[i];
										}
										parseinfo.datamap[di.index()] = std::make_pair(buffer, static_cast<uint32_t>(msize));
									}
									else {
										parseinfo.putErrorLine(PEC_UREscape);
									}
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
				parseinfo.putErrorLine(PEC_URCmd, code);
			}
		}
		else {
			parseinfo.putErrorLine(PEC_URCmd);
		}
	}

	InstStruct::Instruction* parseFuncInstBase(ParseInfo& parseinfo, const std::string &code, const std::vector<std::string> &list);

	void parseFuncInst(ParseInfo &parseinfo, const std::string &code, const std::vector<std::string> &list)
	{
		parseinfo.currfunc_creater.current_line++;
		parseinfo.currfunc_creater.get()->instdata.push_back(parseFuncInstBase(parseinfo, code, list));
	}

	void parseLine(ParseInfo &parseinfo, const std::string &line)
	{
		char fc = line[0];

		if (fc == '.') {
			parseinfo.currfunc_creater.over();
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
		else if (fc == '#') {
			if (parseinfo.currfunc_creater.get() == nullptr) {
				parseinfo.putErrorLine(PEC_URCmd, line);
			}
			ParsedIdentifier labelkey = parseLabelKey(parseinfo, line);
			size_t id = parseinfo.currfunc_creater.labelkeytable[parseinfo.info.hashStringPool.get(labelkey)];
			Config::LineCountType line = parseinfo.currfunc_creater.current_line;
			parseinfo.currfunc_creater.labelkeytable.setLine(id, line);
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
					if (isinst && parseinfo.currfunc_creater.get() == nullptr) {
						parseinfo.putErrorLine();
					}
					else {
						(isinst ? parseFuncInst : parseSectionInside)(parseinfo, cmd, list);
					}
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

		parseinfo.currfunc_creater.over();
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

		const static ParseInstMap parsemap {
			{
				"mov",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) {
					return new Insts::Move(parseRegister(parseinfo, list[0]), parseRegister(parseinfo, list[1]));
				}
			},
			{
				"load",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) -> InstStruct::Instruction* {
					if (list.size() != 3) {
						parseinfo.putErrorLine(PEC_IllegalFormat, "load");
						return nullptr;
					}
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
				"loadp",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) -> InstStruct::Instruction* {
					if (list.size() == 2 && (!list[1].empty() && list[1][0] == '#')) {
						return new Insts::LoadPointer(
							parseRegister(parseinfo, list[0]),
							parseDataIndex(parseinfo, list[1]));
						/*if (!list[1].empty() && list[1][0] != '#') {
							return new Insts::LoadPointer1(
								parseRegister(parseinfo, list[0]),
								parseDataInst(parseinfo, list[1]));
						}
						else {
							return new Insts::LoadPointer2(
								parseRegister(parseinfo, list[0]),
								parseDataIndex(parseinfo, list[1]));
						}*/
					}
					else {
						parseinfo.putErrorLine(PEC_IllegalFormat, "loadp");
						return nullptr;
					}
				}
			},
			{
				"call",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) -> InstStruct::Instruction* {
					if (list.size() >= 2) {
						auto res = parseRegister(parseinfo, list[0]);
						const auto &namekey = parseIdentifier(parseinfo, list[1]);
						auto id = parseinfo.functable.getID(namekey);
						FuncIdentifier func(id);
						ArgumentList::Creater arglist_creater(list.size() - 2);
						for (auto e : PriLib::rangei(list.begin() + 2, list.end())) {
							arglist_creater.push_back(parseRegister(parseinfo, e));
						}
						return new Insts::Call(res, func, arglist_creater.data());
					}
					else {
						parseinfo.putErrorLine(PEC_IllegalFormat, "call");
						return nullptr;
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
				"jump",
				[](ParseInfo &parseinfo, const std::vector<std::string> &list) -> InstStruct::Instruction* {
					if (list.size() == 1) {
						ParsedIdentifier label = parseLabelKey(parseinfo, list[0]);
						size_t id = parseinfo.currfunc_creater.labelkeytable[parseinfo.info.hashStringPool.get(label)];
						assert(id < std::numeric_limits<Config::LineCountType>::max());
						auto *inst = new Insts::Jump(static_cast<Config::LineCountType>(id));
						parseinfo.currfunc_creater.rec_line.push_back(&inst->line);
						return inst;
					}
					else {
						parseinfo.putErrorLine(PEC_IllegalFormat, "jump");
						return nullptr;
					}
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
