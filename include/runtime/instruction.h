#pragma once
#include <functional>

namespace CVM
{
	namespace Runtime
	{
		class Environment;

		class Instruction
		{
		public:
			virtual ~Instruction() {}
			virtual void operator()(Environment &env) const = 0;
		};
	}
}
