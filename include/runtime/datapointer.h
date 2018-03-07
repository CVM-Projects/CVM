#pragma once
#include "../prilib/include/explicittype.h"
#include "typeinfo.h"
#include <cstdint>

namespace CVM
{
	namespace Runtime
	{
		using byte = uint8_t;

		class  ConstDataPointer;

		class DataPointer : private PriLib::ExplicitType<void*, nullptr>
		{
		public:
			explicit DataPointer(void *ptr = nullptr)
				: PriLib::ExplicitType<void*, nullptr>(ptr) {}

			static const MemorySize Size;

			template <typename T = void>
			T* get() const {
				return reinterpret_cast<T*>(data);
			}

			operator ConstDataPointer() const;
			
			template <typename T>
			DataPointer offset(T _offset) const {
				return DataPointer(get<byte>() + _offset);
			}
			DataPointer offset(MemorySize _offset) const {
				return DataPointer(get<byte>() + _offset.data);
			}
		};

		class  ConstDataPointer : private PriLib::ExplicitType<const void*, nullptr>
		{
		public:
			explicit ConstDataPointer(const void *ptr = nullptr)
				: PriLib::ExplicitType<const void*, nullptr>(ptr) {}

			template <typename T = void>
			const T* get() const {
				return reinterpret_cast<const T*>(data);
			}

			template <typename T>
			ConstDataPointer offset(T _offset) const {
				return ConstDataPointer(get<byte>() + _offset);
			}
			ConstDataPointer offset(MemorySize _offset) const {
				return ConstDataPointer(get<byte>() + _offset.data);
			}
		};

		inline DataPointer::operator ConstDataPointer() const {
			return ConstDataPointer(data);
		}
	}
}
