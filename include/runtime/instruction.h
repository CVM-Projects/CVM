#pragma once
#include <functional>

namespace CVM
{
	namespace Runtime
	{
		class Environment;

		using Instruction = std::function<void(Environment &)>;
	}
}
