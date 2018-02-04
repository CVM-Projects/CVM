#pragma once
#include <map>

namespace CVM
{
	using DataPool = std::map<uint32_t, std::pair<uint8_t*, size_t>>;
}
