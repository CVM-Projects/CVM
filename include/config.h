// config.h
// * This file includes all the config for CVM.

#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>

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
		using StackOffsetType = std::int32_t;
		using LineCountType = std::uint32_t;
		using FileIndexType = std::uint32_t;

		constexpr bool isCheckMemorySizeOverflow = true;
		inline void ThrowMemorySizeOverflowError() {
			std::puts("Error occur while plusing two MemorySize.");
			std::exit(1);
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
			return 0 < id && id <= stsize;
		}
		inline RegisterIndexType get_dynamic_id(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			return id - 1;
		}
		inline RegisterIndexType get_static_id(RegisterIndexType id, RegisterIndexType dysize, RegisterIndexType stsize) {
			return id - 1;
		}
	}
}
