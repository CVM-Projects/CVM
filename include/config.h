// config.h
// * This file includes all the config for CVM.

#pragma once
#include <cstdint>

namespace CVM
{
	namespace Config
	{
		using MemorySizeType = std::uint32_t;
		using RegisterIndexType = std::uint16_t;
		using TypeIndexType = std::uint32_t;
		using DataInstType = std::uint32_t;
		using DataIndexType = std::uint32_t;
		using StackSizeType = std::uint32_t;
		using StackOffsetType = std::uint16_t;
	}
}
