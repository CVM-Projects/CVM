#pragma once
#include "explicittype.h"
#include <cstdint>

namespace CVM
{
	namespace Runtime
	{
		using byte = uint8_t;

		class DataPointer : private Common::ExplicitType<void*, nullptr>
		{
		public:
			explicit DataPointer(void *ptr = nullptr)
				: Common::ExplicitType<void*, nullptr>(ptr) {}

			template <typename T = void>
			T* get() const {
				return reinterpret_cast<T*>(data);
			}
			
			template <typename T>
			DataPointer offset(T _offset) const {
				return DataPointer(get<byte>() + _offset);
			}
		};
		using ConstDataPointer = Common::ExplicitType<const void*, nullptr>;
	}
}
