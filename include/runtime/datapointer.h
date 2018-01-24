#pragma once
#include "../prilib/include/explicittype.h"
#include "typeinfo.h"
#include <cstdint>

namespace CVM
{
	namespace Runtime
	{
		using byte = uint8_t;

		class DataPointer : private PriLib::ExplicitType<void*, nullptr>
		{
		public:
			explicit DataPointer(void *ptr = nullptr)
				: PriLib::ExplicitType<void*, nullptr>(ptr) {}

			template <typename T = void>
			T* get() const {
				return reinterpret_cast<T*>(data);
			}
			
			template <typename T>
			DataPointer offset(T _offset) const {
				return DataPointer(get<byte>() + _offset);
			}
			DataPointer offset(MemorySize _offset) const {
				return DataPointer(get<byte>() + _offset.data);
			}
		};
		using ConstDataPointer = PriLib::ExplicitType<const void*, nullptr>;
	}
}
