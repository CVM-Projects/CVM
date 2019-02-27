#include "basic.h"
#include "parser/parse.h"
#include "inststruct/instcode.h"
#include "inststruct/function.h"
#include "inststruct/instpart.h"
#include "inststruct/identkeytable.h"
#include "bignumber.h"
#include "inststruct/info.h"
#include "parser/parse-inststruct.h"
#include <regex>
#include <vector>

namespace CVM
{
	using ParsedIdentifier = HashID;

	enum ParseErrorCode {
#define ParseErrorCode(PEC_Code, Message) PEC_Code,
#include "parse-errorcode.def"
	};

	class ParseInfo
	{
	public:
		explicit ParseInfo(InstStruct::GlobalInfo &globalInfo)
			: currfunc_creater(*this), info(globalInfo), literalDataPoolCreator(*globalInfo.literalDataPoolCreator) {}

		~ParseInfo() {
			//println("~ParseInfo();");
		}

		class FunctionInfoCreater
		{
		public:
			class LabelKeyTable
			{
			public:
				size_t operator[](const HashID &key) {
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
				std::map<HashID, size_t> _data;
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

		InstStruct::GlobalInfo &info;
		LiteralDataPoolCreator &literalDataPoolCreator;
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
#define ParseErrorCode(PEC_Code, Message) { PEC_Code, Message },
#include "parse-errorcode.def"
			};
			return pecmap.at(pec);
		}
	};

	PriLib::StorePtr<ParseInfo> createParseInfo(InstStruct::GlobalInfo &ginfo) {
		return PriLib::StorePtr<ParseInfo>(new ParseInfo(ginfo));
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

	bool isIdentifierEscapePrefixChar(ParseInfo &parseinfo, char c) {
		return c == '%' || c == '#';
	}

	bool hasIdentifierPrefix(ParseInfo &parseinfo, const char *str) {
		if (isIdentifierEscapePrefixChar(parseinfo, str[0]))
			return str[0] == str[1];
		else if (str[0] == '+' || str[0] == '-')
			return !std::isdigit(str[1]);
		else
			return isIdentifierChar(parseinfo, str[0]);
	}

	bool hasSignedNumberPrefix(ParseInfo &parseinfo, const char *str) {
		return std::isdigit(str[0]) || ((str[0] == '+' || str[0] == '-') && std::isdigit(str[1]));
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
				parseinfo.putErrorLine(PEC_URIntegerData);
			});
	}

	template <typename T>
	T parseNumber(ParseInfo &parseinfo, const InstStruct::Element &elt) {
		if (elt.type() != InstStruct::ET_IntegerData) {
			parseinfo.putErrorLine(PEC_URIntegerData);
			return 0;
		}
		const auto & integer = elt.get<InstStruct::IntegerData>();
		if (integer.has_signed_prefix()) {
			parseinfo.putErrorLine(PEC_NumIsSigned, InstStruct::ToString(elt, parseinfo.info));
			return 0;
		}
		if (integer.data().size() > sizeof(T)) {
			parseinfo.putErrorLine(PEC_NumTooLarge, InstStruct::ToString(elt, parseinfo.info));
			return 0;
		}
		return parseNumber<T>(parseinfo, integer.data().toString(10));  // TODO
	}

	template <typename T>
	void parseNumber(ParseInfo &parseinfo, T &result, const InstStruct::Element &elt) {
		result = parseNumber<T>(parseinfo, elt);
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word);

	InstStruct::Identifier parseIdentifier(ParseInfo &parseinfo, const std::string &word) {
		ParseUnit parseunit(parseinfo, word);
		auto result = Parse::Parse<InstStruct::Identifier>(parseunit);
		if (result) {
			return *result;
		}
		else {
			parseinfo.putErrorLine(PEC_UREscape, word);
			return InstStruct::Identifier(HashID(0));
		}
	}

	InstStruct::LineLabel parseLineLabel(ParseInfo &parseinfo, const std::string &word) {
		ParseUnit parseunit(parseinfo, word);
		auto result = Parse::Parse<InstStruct::LineLabel>(parseunit);
		if (result) {
			return *result;
		}
		else {
			parseinfo.putErrorLine(PEC_URLineLabel, word);
			return InstStruct::LineLabel(HashID(0));
		}
	}

	void parseDataLarge(ParseInfo &parseinfo, const InstStruct::IntegerData &data, uint8_t *buffer, size_t size) {
		if (data.has_signed_prefix()) {
			parseinfo.putErrorLine(PEC_NumIsSigned, InstStruct::ToString(data, parseinfo.info));
			return;
		}
		const BigInteger &bi = data.data();
		bool is_large;
		if (!bi.toBufferLSB(buffer, size, is_large)) {
			if (is_large) {
				parseinfo.putErrorLine(PEC_NumTooLarge, InstStruct::ToString(data, parseinfo.info));
				parseinfo.putError("The accepted size is " + std::to_string(size) + " byte(s).");
			} else {
				parseinfo.putErrorLine(PEC_URIntegerData, InstStruct::ToString(data, parseinfo.info));
			}
		}
	}
	bool parseDataLarge(ParseInfo &parseinfo, const InstStruct::IntegerData &data, std::function<uint8_t*(size_t)> creater) {
		if (data.has_signed_prefix()) {
			parseinfo.putErrorLine(PEC_NumIsSigned, InstStruct::ToString(data, parseinfo.info));
			return false;
		}
		const BigInteger &bi = data.data();
		size_t size = bi.size();
		uint8_t *buffer = creater(size);
		if (bi.toBufferLSB(buffer, size)) {
			return true;
		} else {
			parseinfo.putErrorLine(PEC_URIntegerData, InstStruct::ToString(data, parseinfo.info));
		}
		return false;
	}

	TypeIndex parseType(ParseInfo &parseinfo, const std::string &word) {
		TypeIndex index;
		if (parseinfo.info.typeInfoMap.find(parseIdentifier(parseinfo, word).data(), index)) {
			return index;
		}
		else {
			parseinfo.putErrorLine(PEC_UFType, word);
			return TypeIndex(0);
		}
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
}

namespace CVM
{
	template <typename ET>
	const ET& getFromElement(const InstStruct::Element &elt) {
		if (elt.type() != ET::elementType) {
			assert(false);
		}
		return elt.get<ET>();
	}

	bool check(const std::vector<InstStruct::Element> &list, const std::vector<InstStruct::ElementType> &et) {
		if (list.size() != et.size())
			return false;
		for (size_t i = 0; i != list.size(); ++i) {
			if (list[i].type() != et[i])
				return false;
		}
		return true;
	}
	void checkAndPutError(ParseInfo& parseinfo, ParseErrorCode pec, const std::vector<InstStruct::Element> &list, const std::vector<InstStruct::ElementType> &et) {
		if (!check(list, et)) {
			parseinfo.putErrorLine(pec);
		}
	}

	bool convert(ParseInfo &parseinfo, const std::vector<std::string> &list, std::vector<InstStruct::Element> &result) {
		bool success = true;

		for (auto &elt : list) {
			ParseUnit parseunit(parseinfo, elt);
			auto res = Parse::Parse<InstStruct::Element>(parseunit);
			if (!res) {
				ParseErrorCode pec;
				switch (parseunit.errorcode) {
#define InstPart(key) case InstStruct::ET_##key: pec = PEC_UR##key; break;
#include "inststruct/instpart.def"
				default: pec = PEC_URElement; break;
				}
				parseinfo.putErrorLine(pec, elt);
				success = false;
			}
			else {
				result.emplace_back(std::move(*res));
			}
		}

		return success;
	}

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
			const auto &namekey = parseIdentifier(parseinfo, list.at(0)).data();
			auto &data = parseinfo.info.funcTable.getData(namekey);
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
			const auto &nameid = parseIdentifier(parseinfo, list.at(0)).data();
			TypeIndex tid;
			if (parseinfo.info.typeInfoMap.find(nameid, tid)) {
				parseinfo.putErrorLine(PEC_DUType);
			}
			else {
				parseinfo.currtype = nameid;
				parseinfo.info.typeInfoMap.insert(nameid, TypeInfo());
			}
			break;
		}
		}
	}

	static uint8_t* createMemory(ParseInfo &parseinfo, MemorySize size) {
		return parseinfo.literalDataPoolCreator.alloc(size);
	}

	void parseSectionInside(ParseInfo &parseinfo, const std::string &code, const std::vector<std::string> &list) {
		using ParseInsideProcess = std::function<void(ParseInfo &, const std::vector<InstStruct::Element> &)>;
		using ParseInsideMap = std::map<std::string, ParseInsideProcess>;
		const static std::map<int, ParseInsideMap> parsemap {
			{
				ks_func,
				ParseInsideMap {
					{
						"arg",
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (check(list, {})) {
							}
							else {
								if (list[0].type() == InstStruct::ET_Register) {
									for (const auto &elt : list) {
										if (elt.type() == InstStruct::ET_Register) {
											auto &reg = getFromElement<InstStruct::Register>(elt);
											if (reg.isPrivateDataRegister())
												parseinfo.currfunc_creater.arglist.push_back(reg.index());
											else
												parseinfo.putErrorLine(PEC_NotFuncReg, InstStruct::ToString(elt, parseinfo.info));
										}
										else {
											parseinfo.putErrorLine();
											return;
										}
									}
								}
								else {
									if (check(list, { InstStruct::ET_IntegerData })) {
										auto count = parseNumber<Config::RegisterIndexType>(parseinfo, list[0]);
										if (parseinfo.info.dataRegisterMode == InstStruct::drm_multiply) {
											if (count != 0) {
												parseinfo.putErrorLine(PEC_ModeNotAllow, InstStruct::ToString(list[0], parseinfo.info));
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (list.size() == 1) {
								TypeIndex index = parseType(parseinfo, InstStruct::ToString(list[0], parseinfo.info));
								parseinfo.currfunc_creater.restype = index;
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"dyvarb",
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (list.size() == 2) {
								auto &type = list[list.size() - 1];
								size_t count = parseNumber<size_t>(parseinfo, list[0]);
								TypeIndex index = parseType(parseinfo, InstStruct::ToString(type, parseinfo.info));
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (list.size() == 1) {
								checkAndPutError(parseinfo, PEC_URIdentifier, list, { InstStruct::ET_Identifier });
								parseinfo.info.entry = getFromElement<InstStruct::Identifier>(list[0]).data();
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					},
					{
						"mode",
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (list.size() == 1) {
								checkAndPutError(parseinfo, PEC_URIdentifier, list, { InstStruct::ET_Identifier });
								std::string str = InstStruct::ToString(list[0], parseinfo.info);
								if (str == "multiply") {
									parseinfo.info.dataRegisterMode = InstStruct::drm_multiply;
								}
								else if (str == "dynamic") {
									parseinfo.info.dataRegisterMode = InstStruct::drm_dynamic;
								}
								else if (str == "static") {
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							const auto &nameid = parseinfo.currtype;
							auto &typeinfo = parseinfo.info.typeInfoMap.at(nameid);
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (check(list, { InstStruct::ET_DataLabel, InstStruct::ET_IntegerData, InstStruct::ET_IntegerData }) ||
							    check(list, { InstStruct::ET_DataLabel, InstStruct::ET_IntegerData })) {
								const auto &di = getFromElement<InstStruct::DataLabel>(list[0]);
								if (!parseinfo.literalDataPoolCreator.has(FileID(0), DataID(di.data()))) {
									uint8_t *buffer = nullptr;
									size_t msize = 0;
									const auto &dat = getFromElement<InstStruct::IntegerData>(list[1]);
									if (list.size() == 3) {
										msize = parseNumber<size_t>(parseinfo, list[2]);
										buffer = createMemory(parseinfo, MemorySize(msize));
										parseDataLarge(parseinfo, dat, buffer, msize);
									}
									else {
										if (!parseDataLarge(parseinfo, dat, [&](size_t size) {
											msize = size;
											return buffer = createMemory(parseinfo, MemorySize(size));
										}))
											delete[] buffer;
									}
									parseinfo.literalDataPoolCreator.insert((FileID(0), DataID(di.data())), std::make_pair(MemorySize(msize), buffer)); // TODO
//									parseinfo.datamap[di.index()] = std::make_pair(buffer, static_cast<uint32_t>(msize));
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (check(list, { InstStruct::ET_DataLabel, InstStruct::ET_ArrayData })) {
								const auto &di = getFromElement<InstStruct::DataLabel>(list[0]);
								if (!parseinfo.literalDataPoolCreator.has(FileID(0), DataID(di.data()))) {
									const auto &data = getFromElement<InstStruct::ArrayData>(list[1]);
									auto &vec = data.data();
									uint8_t *buffer = createMemory(parseinfo, MemorySize(vec.size()));
									PriLib::Memory::copyTo(buffer, vec.data(), vec.size());
									parseinfo.literalDataPoolCreator.insert((FileID(0), DataID(di.data())), std::make_pair(MemorySize(static_cast<uint32_t>(vec.size())), buffer)); // TODO
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
						[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
							if (check(list, { InstStruct::ET_DataLabel, InstStruct::ET_String })) {
								const auto &di = getFromElement<InstStruct::DataLabel>(list[0]);
								if (!parseinfo.literalDataPoolCreator.has(FileID(0), DataID(di.data()))) {
									const auto &str = getFromElement<InstStruct::String>(list[1]);
									const std::string &nword = str.data();
									size_t msize = nword.size() + 1;
									uint8_t *buffer = createMemory(parseinfo, MemorySize(msize));
									for (size_t i = 0; i < nword.size(); ++i) {
										buffer[i] = (uint8_t)nword[i];
									}
									parseinfo.literalDataPoolCreator.insert((FileID(0), DataID(di.data())), std::make_pair(MemorySize(msize), buffer)); // TODO
								}
								else {
									parseinfo.putErrorLine(PEC_DUDataId);
								}
							}
							else {
								parseinfo.putErrorLine();
							}
						}
					}
				},
			},
		};

		// Convert to InstStruct::Element
		std::vector<InstStruct::Element> eltlist;

		if (!convert(parseinfo, list, eltlist))
			return;

		auto iter = parsemap.find(parseinfo.currsection);
		if (iter != parsemap.end()) {
			auto iiter = iter->second.find(code);
			if (iiter != iter->second.end()) {
				iiter->second(parseinfo, eltlist);
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
			InstStruct::LineLabel labelkey = parseLineLabel(parseinfo, line);
			size_t id = parseinfo.currfunc_creater.labelkeytable[labelkey.data()];
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
		using ParseInstProcess = std::function<InstStruct::Instruction*(ParseInfo &, const std::vector<InstStruct::Element> &)>;
		using ParseInstMap = std::map<std::string, ParseInstProcess>;

		using namespace InstStruct;

		const static ParseInstMap parsemap {
			{
				"mov",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
					checkAndPutError(parseinfo, PEC_URIns, list, { ET_Register, ET_Register });
					return new Insts::Move(getFromElement<InstStruct::Register>(list[0]), getFromElement<InstStruct::Register>(list[1]));
				}
			},
			{
				"load",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) -> InstStruct::Instruction* {
					if (check(list, { ET_Register, ET_IntegerData, ET_Identifier })) {
						return new Insts::Load1(
							getFromElement<InstStruct::Register>(list[0]),
							InstStruct::Data(parseNumber<InstStruct::Data::Type>(parseinfo, list[1])),
							parseType(parseinfo, ToString(list[2], parseinfo.info)));
					}
					else if (check(list, { ET_Register, ET_DataLabel, ET_Identifier })) {
						return new Insts::Load2(
							getFromElement<InstStruct::Register>(list[0]),
							getFromElement<InstStruct::DataLabel>(list[1]),
							parseType(parseinfo, ToString(list[2], parseinfo.info)));
					}
					else {
						parseinfo.putErrorLine(PEC_IllegalFormat, "load");
						return nullptr;
					}
				}
			},
			{
				"loadp",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) -> InstStruct::Instruction* {
					if (check(list, { ET_Register, ET_DataLabel })) {
						return new Insts::LoadPointer(
							getFromElement<InstStruct::Register>(list[0]),
							getFromElement<InstStruct::DataLabel>(list[1]));
					}
					else {
						parseinfo.putErrorLine(PEC_IllegalFormat, "loadp");
						return nullptr;
					}
				}
			},
			{
				"call",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) -> InstStruct::Instruction* {
					if (list.size() >= 2) {
						if (list[0].type() == ET_Register && list[1].type() == ET_Identifier) {
							auto res = getFromElement<InstStruct::Register>(list[0]);
							const auto &namekey = getFromElement<InstStruct::Identifier>(list[1]).data();
							auto id = parseinfo.info.funcTable.getID(namekey);
							FuncIdentifier func(id);
							ArgumentList::creater arglist_creater(list.size() - 2);
							for (auto &e : PriLib::rangei(list.begin() + 2, list.end())) {
								if (e.type() != ET_Register) {
									parseinfo.putErrorLine(PEC_URRegister, "Not register");
									return nullptr;
								}
								arglist_creater.push_back(getFromElement<InstStruct::Register>(e));
							}
							return new Insts::Call(res, func, arglist_creater.data());
						}
					}
					parseinfo.putErrorLine(PEC_IllegalFormat, "call");
					return nullptr;
				}
			},
			{
				"ret",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
					return new Insts::Return();
				}
			},
			{
				"jump",
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) -> InstStruct::Instruction* {
					if (list.size() == 1) {
						InstStruct::LineLabel label = getFromElement<InstStruct::LineLabel>(list[0]);
						size_t id = parseinfo.currfunc_creater.labelkeytable[label.data()];
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
				[](ParseInfo &parseinfo, const std::vector<InstStruct::Element> &list) {
					return new Insts::Debug_OutputRegister();
				}
			},
		};

		// Convert to InstStruct::Element
		std::vector<InstStruct::Element> eltlist;

		if (!convert(parseinfo, list, eltlist))
			return nullptr;

		auto iter = parsemap.find(code);
		if (iter != parsemap.end()) {
			return iter->second(parseinfo, eltlist);
		}
		else {
			parseinfo.putErrorLine(PEC_URIns, code);
			return nullptr;
		}
	}
}
