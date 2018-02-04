#pragma once
#include <memory>

namespace CVM
{
	class BigInteger
	{
	public:
		class Data;
	public:
		explicit BigInteger();

		bool parse(const std::string &word, int base = 0);
		std::string toString(int base = 10) const;
		size_t size() const;

		template <typename T>
		bool loadData(const T &src) {
			if (is_lsb_system()) return loadDataLSB(src);
			else return loadDataMSB(src);
		}
		bool loadFromBuffer(const void *buffer, size_t memsize) {
			if (is_lsb_system()) return loadFromBufferLSB(buffer, memsize);
			else return loadFromBufferMSB(buffer, memsize);
		}
		bool toBuffer(void *buffer, size_t memsize) const {
			if (is_lsb_system()) return toBufferLSB(buffer, memsize);
			else return toBufferMSB(buffer, memsize);
		}
		bool toBuffer(void *buffer, size_t memsize, bool &is_large) const {
			if (is_lsb_system()) return toBufferLSB(buffer, memsize, is_large);
			else return toBufferMSB(buffer, memsize, is_large);
		}

		bool loadFromBufferLSB(const void *buffer, size_t memsize);
		template <typename T>
		bool loadDataLSB(const T &src) {
			return loadFromBufferLSB(&src, sizeof(T));
		}

		bool loadFromBufferMSB(const void *buffer, size_t memsize);
		template <typename T>
		bool loadDataMSB(const T &src) {
			return loadFromBufferMSB(&src, sizeof(T));
		}
		
		bool toBufferLSB(void *buffer, size_t memsize) const;
		bool toBufferLSB(void *buffer, size_t memsize, bool &is_large) const;

		bool toBufferMSB(void *buffer, size_t memsize) const;
		bool toBufferMSB(void *buffer, size_t memsize, bool &is_large) const;

	private:
		std::shared_ptr<Data> data;
		static bool is_lsb_system();
	};
}
