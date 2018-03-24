// config.h
// * This file includes all the config for CVM.

#pragma once
#include <cstdint>
#include <cstdio>

namespace CVM
{
	namespace Config
	{
		// Type Define

		using MemorySizeType = std::uint32_t;
		using MemoryCountType = std::uint32_t;
		using RegisterIndexType = std::uint32_t;
		using TypeIndexType = std::uint32_t;
		using DataInstType = std::uint32_t;
		using DataIndexType = std::uint32_t;
		using StackSizeType = std::uint32_t;
		using FuncIndexType = std::uint32_t;
		using StackOffsetType = std::uint16_t;
		using LineCountType = std::uint32_t;

		constexpr bool isCheckMemorySizeOverflow = true;
		inline void ThrowMemorySizeOverflowError() {
			puts("Error occur while plusing two MemorySize.");
			exit(1);
		}
		inline void CheckMemorySizeOverflow(bool pass) {
			if (isCheckMemorySizeOverflow && !pass) {
				ThrowMemorySizeOverflowError();
			}
		}

		// Register Set

		inline bool is_dynamic(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			return 0 < id && id <= dysize;
		}
		inline bool is_static(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			return dysize < id && id <= dysize + stsize;
		}
		inline RegisterIndexType get_dynamic_id(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			assert(id >= 1);
			return id - 1;
		}
		inline RegisterIndexType get_static_id(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			assert(id >= dysize + 1);
			return id - dysize - 1;
		}
		template <typename T>
		Config::RegisterIndexType convertToRegisterIndexType(T value) {
			assert(value <= std::numeric_limits<RegisterIndexType>::max());
			return static_cast<RegisterIndexType>(value);
		}
	}
}
