#include "basic.h"
#include "bignumber.h"
#include <gmpxx.h>

namespace CVM
{
	class BigInteger::Data
	{
	public:
		mpz_class data;
	};

	static bool parseBigInteger(const std::string &word, mpz_class &result, int base);

	BigInteger::BigInteger()
		: data(new Data()) {}

	bool BigInteger::parse(const std::string &word, int base) {
		return parseBigInteger(word, data->data, base);
	}
	std::string BigInteger::toString(int base) const {
		if (base > 1 && base <= 36)
			return data->data.get_str(base);
		else
			return "";
	}

	static size_t get_msize(const std::string &str) {
		return str.size() / 2 + str.size() % 2;
	}

	size_t BigInteger::size() const {
		return get_msize(toString(16));
	}

	bool BigInteger::loadFromBufferLSB(const void *buffer, size_t memsize) {
		PriLib::charptr cp(memsize * 2);

		for (size_t i = 0; i != memsize; ++i) {
			uint8_t v = (reinterpret_cast<const uint8_t*>(buffer))[i];
			cp.set((memsize - i - 1) * 2, PriLib::Convert::to_hex(v));
		}

		data->data.set_str(cp.to_string(), 16);

		return true;
	}
	bool BigInteger::loadFromBufferMSB(const void *buffer, size_t memsize) {
		PriLib::charptr cp(memsize * 2);

		for (size_t i = 0; i != memsize; ++i) {
			uint8_t v = (reinterpret_cast<const uint8_t*>(buffer))[i];
			cp.set(i * 2, PriLib::Convert::to_hex(v));
		}

		data->data.set_str(cp.to_string(), 16);

		return true;
	}

	bool BigInteger::toBufferLSB(void *buffer, size_t memsize) const {
		bool is_large;
		return toBufferLSB(buffer, memsize, is_large);
	}
	bool BigInteger::toBufferMSB(void *buffer, size_t memsize) const {
		bool is_large;
		return toBufferMSB(buffer, memsize, is_large);
	}
	bool BigInteger::toBufferLSB(void *buffer, size_t memsize, bool &is_large) const {
		std::string str = toString(16);
		if (get_msize(str) > memsize) {
			is_large = true;
			return false;
		}
		return PriLib::Convert::to_base16(str, buffer, memsize, 0);
	}
	bool BigInteger::toBufferMSB(void *buffer, size_t memsize, bool &is_large) const {
		std::string str = toString(16);
		if (get_msize(str) > memsize) {
			is_large = true;
			return false;
		}
		return PriLib::Convert::to_base16(str, buffer, memsize, memsize);
	}

	bool BigInteger::is_lsb_system()
	{
		union {
			uint16_t v;
			uint8_t d[2];
		};

		v = 0x0100;

		return d[0] == 0;
	}

	static bool parseBigInteger(const std::string &word, mpz_class &result, int base) {
		if (word.empty()) {
			return false;
		}
		const char *pword = word.c_str();
		size_t size = word.size();
		bool ispositive = true;

		if (pword[0] == '+' || pword[0] == '-') {
			if (pword[0] == '-') {
				ispositive = false;
			}
			pword++;
			size--;
		}

		if (base == 0) {
			if (pword[0] == '0') {
				if (size == 1) {
					result = 0;
					return true;
				}
				else if (size == 2) {
					pword++;
					base = 8;
				}
				else {
					switch (pword[1]) {
					case 'b': base = 2; pword += 2; break;
					case 'o': base = 8; pword += 2; break;
					case 'd': base = 10; pword += 2; break;
					case 'x': base = 16; pword += 2; break;
					default: base = 8; pword++; break;
					}
				}
			}
			else {
				base = 10;
			}
		}

		if (PriLib::Convert::is_integer(pword, base)) {
			result = mpz_class(pword, base);
			if (!ispositive) {
				result = -result;
			}
			return true;
		}

		return false;
	}
}
