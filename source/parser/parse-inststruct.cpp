#include "basic.h"
#include "parser/parse-inststruct.h"
#include "parser/parse.h"
#include <climits>

namespace CVM
{
	namespace Parse
	{
		using namespace InstStruct;

		static bool ParseRegisterScopePrefix(ParseUnit &parseunit, RegisterScopeType &scopetype) {
			// Parse '%t', '%g', '%'
			if (!matchPrefix(parseunit, "%"))
				return false;
			switch (parseunit.currview[0]) {
			case 't': scopetype = rst_thread; parseunit.currview += 1; break;
			case 'g': scopetype = rst_global; parseunit.currview += 1; break;
			default: scopetype = rst_local; break;
			}
			return true;
		}
		static bool ParseStackRegisterPrefix(ParseUnit &parseunit, RegisterScopeType &scopetype) {
			// Parse '%tsp', '%gsp', '%sp'
			return ParseRegisterScopePrefix(parseunit, scopetype) && matchPrefix(parseunit, "sp");
		}
		template <typename NumType>
		static bool ParseNumber(ParseUnit &parseunit, NumType &result, bool allow_negative = false) {
			const char *ptr = parseunit.currview.get();
			const char *endptr = parseunit.currview.get();
			if ((allow_negative && (*ptr == '-')) || (*ptr == '+'))
				++endptr;
			while (isdigit(*endptr))
				++endptr;
			if (ptr == endptr)
				return false;
			if (!PriLib::Convert::to_integer<NumType>(PriLib::StringView(ptr, endptr).toString() /* TODO */, result))
				return false;
			parseunit.currview = PriLib::CharPtrView(endptr);
			return true;
		}
		static bool ParseStackOffset(ParseUnit &parseunit, StackOffset &result) {
			return matchPrefix(parseunit, '[') && ParseNumber(parseunit, result.data, true) && matchPrefix(parseunit, ']');
		}
		static bool IsEndChar(const ParseUnit &parseunit) {
			return isEndChar(parseunit.parseinfo, parseunit.currview[0]);
		}
		static bool ParseIdentifier(ParseUnit &parseunit, HashID &result) {
			if (!hasIdentifierPrefix(parseunit.parseinfo, parseunit.currview.get()))
				return false;
			PriLib::CharPtrView::OffsetType begin = isIdentifierEscapePrefixChar(parseunit.parseinfo, parseunit.currview[0]) ? 1 : 0;
			PriLib::CharPtrView::OffsetType end = begin;
			while (isIdentifierChar(parseunit.parseinfo, parseunit.currview[end]))
				++end;
			PriLib::StringView identstr(parseunit.currview, begin, end);
			result = getGlobalInfo(parseunit.parseinfo).hashStringPool.insert(identstr);
			parseunit.currview += end;
			return true;
		}
		static bool ParseByte(char c1, char c2, uint8_t &result) {
			if (std::isxdigit(c1) && std::isxdigit(c2)) {
				int a = std::isdigit(c1) ? (c1 - '0') : (std::tolower(c1) - 'a' + 0xa);
				int b = std::isdigit(c2) ? (c2 - '0') : (std::tolower(c2) - 'a' + 0xa);
				int num = a * 0x10 + b;
				assert(0 <= num && num <= 0xff);
				result = static_cast<uint8_t>(num);
				return true;
			}
			return false;
		}
		static bool ParseString(ParseUnit &parseunit, std::string &result) {
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

			const char *word = parseunit.currview.get();
			std::string nword;
			int mode = 0;
			if (word[0] != '"')
				return false;
			++word;
			for (char c = *word; c != '\"'; c = *(++word)) {
				if (c == '\0') {
					return false;
				}
				if (mode == 0) {
					if (c == '\\') {
						if (word[1] == '"') {
							nword.push_back('"');
							++word;
						}
						else {
							mode = 1;
						}
					}
					else {
						nword.push_back(c);
					}
				}
				else if (mode == 1) {
					if (c >= '0' && c <= '7') {
						mode = 2;
						word--;
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
				else if (mode == 2) {
					// \X
					// \XX
					// \XXX
					unsigned int num = 0;
					int pos = -1;
					unsigned int pow = 1;
					for (int i = 0; i != 3; ++i) {
						if ('0' <= word[i] && word[i] <= '7') {
							unsigned int res = num + (word[i] - '0') * pow;
							pow *= 010;
							if (res > 0xff) {
								break;
							}
							num = res;
							pos = i;
						}
						else {
							break;
						}
					}
					if (pos == -1) {
						return false;
					}
					assert(num <= 0xff);
					nword.push_back(static_cast<char>(num));
					word += pos;
					mode = 0;
				}
				else if (mode == 3) {
					// \xXX
					uint8_t num = 0;
					if (ParseByte(word[0], word[1], num)) {
						nword.push_back(static_cast<char>(num));
						word += 2 - 1;
						mode = 0;
					}
					else {
						return false;
					}
				}
			}
			if (mode != 0) {
				return false;
			}

			result = nword;
			parseunit.currview = PriLib::CharPtrView(word);

			return true;
		}

		//---------------------------------------------------------------------------------------------
		// * Register
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<ZeroRegister> Parse<ZeroRegister>(ParseUnit &parseunit) {
			ZeroRegister result;
			if (!matchPrefix(parseunit, "%0"))
				return std::nullopt;
			if (!IsEndChar(parseunit))
				return std::nullopt;
			return result;
		}
		template <>
		std::optional<ResultRegister> Parse<ResultRegister>(ParseUnit &parseunit) {
			ResultRegister result;
			if (!matchPrefix(parseunit, "%res"))
				return std::nullopt;
			if (!IsEndChar(parseunit))
				return std::nullopt;
			return result;
		}
		template <>
		std::optional<DataRegister> Parse<DataRegister>(ParseUnit &parseunit) {
			DataRegister result;
			// Get The First Character
			if (!ParseRegisterScopePrefix(parseunit, result.scopeType))
				return std::nullopt;
			// Get Index
			if (!ParseNumber(parseunit, result.registerIndex.data))
				return std::nullopt;
			if (result.registerIndex.data == 0)
				return std::nullopt;
			// Get Data Register Type
			if (IsEndChar(parseunit)) {
				switch (getGlobalInfo(parseunit.parseinfo).dataRegisterMode) {
				case drm_multiply:
					return std::nullopt;
				case drm_dynamic:
					result.registerType = rt_data_dynamic;
					break;
				case drm_static:
					result.registerType = rt_data_static;
					break;
				default:
					return std::nullopt;
				}
			}
			else {
				switch (parseunit.currview[0]) {
				case 'd':
					result.registerType = rt_data_dynamic;
					parseunit.currview += 1;
					break;
				case 's':
					result.registerType = rt_data_static;
					parseunit.currview += 1;
					break;
				default:
					return std::nullopt;
				}
				if (!IsEndChar(parseunit))
					return std::nullopt;
			}
			return result;
		}
		template <>
		std::optional<StackPointerRegister> Parse<StackPointerRegister>(ParseUnit &parseunit) {
			StackPointerRegister result;
			// Get The Prefix
			if (!ParseStackRegisterPrefix(parseunit, result.scopeType))
				return std::nullopt;
			if (!IsEndChar(parseunit)) {
				// Get Offset
				if (!ParseStackOffset(parseunit, result.offset))
					return std::nullopt;
				if (!IsEndChar(parseunit))
					return std::nullopt;
			}
			return result;
		}
		template <>
		std::optional<StackSpaceRegister> Parse<StackSpaceRegister>(ParseUnit &parseunit) {
			StackSpaceRegister result;
			// Get The Prefix
			if (!ParseStackRegisterPrefix(parseunit, result.scopeType))
				return std::nullopt;
			if (parseunit.currview[0] == '[') {
				// Get Offset
				if (!ParseStackOffset(parseunit, result.offset))
					return std::nullopt;
			}
			if (!matchPrefix(parseunit, '('))
				return std::nullopt;
			if (parseunit.currview[0] == ')') {
				result.registerType = rt_stack_space_full;
			}
			else if (isdigit(parseunit.currview[0])) {
				result.registerType = rt_stack_space_size;
				// Get Size
				if (!ParseNumber(parseunit, result.stacksize.data))
					return std::nullopt;
			}
			else if (ParseIdentifier(parseunit, result.typehashid)) {
				result.registerType = rt_stack_space_type;
			}
			if (!matchPrefix(parseunit, ')'))
				return std::nullopt;
			if (matchPrefix(parseunit, '!')) {
				if (result.offset.data != 0) {
					return std::nullopt;
				}
				if (result.registerType == rt_stack_space_size)
					result.registerType = rt_stack_space_size_decrease;
				else if (result.registerType == rt_stack_space_type)
					result.registerType = rt_stack_space_type_decrease;
				else
					return std::nullopt;
			}
			if (!IsEndChar(parseunit))
				return std::nullopt;
			return result;
		}
		template <typename RT>
		static std::optional<Register> _parseRegisterBase(ParseUnit &parseunit) {
			auto parseresult = Parse<RT>(parseunit);
			return parseresult ? std::optional<Register>(Register(*parseresult)) : std::nullopt;
		}
		template <>
		std::optional<Register> Parse<Register>(ParseUnit &parseunit) {
			using Func = std::optional<Register> (ParseUnit &parseunit);
			if (parseunit.currview[0] != '%') {
				return std::nullopt;
			}
			char prefixchar = parseunit.currview[1];
			if (prefixchar == 't' || prefixchar == 'g') {
				prefixchar = parseunit.currview[2];
			}
			if (std::isdigit(prefixchar)) {
				ParseUnit record(parseunit);
				std::optional<Register> result;
				if (prefixchar == '0') {
					result = _parseRegisterBase<ZeroRegister>(record);
				}
				else {
					result = _parseRegisterBase<DataRegister>(record);
				}
				if (result) {
					parseunit = record;
					return result;
				}
				return std::nullopt;
			}
			else if (prefixchar == 'r') {
				ParseUnit record(parseunit);
				std::optional<Register> result = _parseRegisterBase<ResultRegister>(record);
				if (result) {
					parseunit = record;
					return result;
				}
				return std::nullopt;
			}
			else if (prefixchar == 's') {
				static Func* funcs[] = {
					_parseRegisterBase<StackPointerRegister>,
					_parseRegisterBase<StackSpaceRegister>
				};
				for (auto func : funcs) {
					ParseUnit record(parseunit);
					auto result = func(record);
					if (result) {
						parseunit = record;
						return result;
					}
				}
				return std::nullopt;
			}
			return std::nullopt;
		}

		//---------------------------------------------------------------------------------------------
		// * Identifier
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<Identifier> Parse<Identifier>(ParseUnit &parseunit) {
			HashID hashid;
			if (ParseIdentifier(parseunit, hashid)) {
				return Identifier(hashid);
			}
			return std::nullopt;
		}

		//---------------------------------------------------------------------------------------------
		// * String
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<String> Parse<String>(ParseUnit &parseunit) {
			std::string result;
			if (ParseString(parseunit, result)) {
				return String(std::move(result));
			}
			return std::nullopt;
		}

		//---------------------------------------------------------------------------------------------
		// * DataLabel
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<DataLabel> Parse<DataLabel>(ParseUnit &parseunit) {
			// Parse '#'
			if (!matchPrefix(parseunit, "#"))
				return std::nullopt;
			DataLabel::Type result;
			if (ParseNumber(parseunit, result)) {
				return DataLabel(result);
			}
			return std::nullopt;
		}

		//---------------------------------------------------------------------------------------------
		// * LineLabel
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<LineLabel> Parse<LineLabel>(ParseUnit &parseunit) {
			// Parse '#'
			if (!matchPrefix(parseunit, "#"))
				return std::nullopt;
			if (!(std::isalpha(parseunit.currview[0]) || (parseunit.currview[0] == '_')))
				return std::nullopt;
			const char *endptr = parseunit.currview.get();
			while (isalnum(*endptr) || (*endptr == '_'))
				++endptr;
			PriLib::StringView result(parseunit.currview, PriLib::CharPtrView(endptr));
			parseunit.currview = PriLib::CharPtrView(endptr);
			if (!IsEndChar(parseunit))
				return std::nullopt;
			return LineLabel(getGlobalInfo(parseunit.parseinfo).hashStringPool.insert(result));
		}

		//---------------------------------------------------------------------------------------------
		// * ArrayData
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<ArrayData> Parse<ArrayData>(ParseUnit &parseunit) {
			// Parse '0:'
			if (!matchPrefix(parseunit, "0:"))
				return std::nullopt;
			std::vector<uint8_t> data;
			const char *endptr = parseunit.currview.get();
			do {
				uint8_t result = 0;
				if (ParseByte(endptr[0], endptr[1], result)) {
					data.push_back(result);
					endptr += 2;
				}
				else {
					return std::nullopt;
				}
			} while (*endptr == ':' && ++endptr);
			parseunit.currview = PriLib::CharPtrView(endptr);
			if (!IsEndChar(parseunit))
				return std::nullopt;
			return ArrayData(std::move(data));
		}

		//---------------------------------------------------------------------------------------------
		// * IntegerData
		//---------------------------------------------------------------------------------------------
		template <>
		std::optional<IntegerData> Parse<IntegerData>(ParseUnit &parseunit) {
			if (!hasSignedNumberPrefix(parseunit.parseinfo, parseunit.currview.get()))
				return std::nullopt;
			bool has_signed_prefix = false;
			const char *endptr = parseunit.currview.get();
			std::string word;
			if (*endptr == '+' || *endptr == '-') {
				has_signed_prefix = true;
				word.push_back(*endptr);
				endptr++;
			}
			if ((endptr[0] == '0') && ((endptr[1] == 'x' || endptr[1] == 'd' || endptr[1] == 'o' || endptr[1] == 'b') || std::isdigit(endptr[1]))) {
				word.push_back(endptr[0]);
				word.push_back(endptr[1]);
				endptr += 2;
			}
			while (*endptr) {
				if (std::isxdigit(*endptr)) {
					word.push_back(*endptr);
					endptr++;
				}
				else if (*endptr == '_') {  // Ignore '_' character
					endptr++;
				}
				else {
					break;
				}
			}
			parseunit.currview = PriLib::CharPtrView(endptr);
			if (!IsEndChar(parseunit))
				return std::nullopt;
			BigInteger data;
			if (has_signed_prefix)
				data.parse(word);
			else
				data.parseu(word);
			return IntegerData(std::move(data), has_signed_prefix);
		}

		//---------------------------------------------------------------------------------------------
		// * Element
		//---------------------------------------------------------------------------------------------
		template <typename T>
		static std::optional<Element> _parseElementBase(ParseUnit &parseunit) {
			auto result = Parse<T>(parseunit);
			if (!result) {
				parseunit.errorcode = static_cast<int>(T::elementType);
				return std::nullopt;
			}
			return std::optional<Element>(Element(std::move(*result)));
		}
		template <>
		std::optional<Element> Parse<Element>(ParseUnit &parseunit) {
			if (parseunit.currview[0] == '%' && parseunit.currview[1] != '%') {
				return _parseElementBase<Register>(parseunit);
			}
			else if (parseunit.currview[0] == '#' && parseunit.currview[1] != '#') {
				if (std::isdigit(parseunit.currview[1])) {
					return _parseElementBase<DataLabel>(parseunit);
				}
				else {
					return _parseElementBase<LineLabel>(parseunit);
				}
			}
			else if (parseunit.currview[0] == '"') {
				return _parseElementBase<String>(parseunit);
			}
			else if (hasSignedNumberPrefix(parseunit.parseinfo, parseunit.currview.get())) {
				if (parseunit.currview[0] == '0' && parseunit.currview[1] == ':') {
					return _parseElementBase<ArrayData>(parseunit);
				}
				else {
					return _parseElementBase<IntegerData>(parseunit);
				}
			}
			else if (hasIdentifierPrefix(parseunit.parseinfo, parseunit.currview.get())){
				return _parseElementBase<Identifier>(parseunit);
			}
			return std::nullopt;
		}
	}
}
