#pragma once
#include "runtime/environment.h"

namespace CVM
{
	class VirtualMachine
	{
	public:
		VirtualMachine() {}

		template <typename... Args>
		void addGlobalEnvironment(Args&&... args) {
			_genv = std::shared_ptr<Runtime::GlobalEnvironment>(new Runtime::GlobalEnvironment(args...));
		}

		Runtime::GlobalEnvironment& Genv() {
			return *_genv;
		}

		void Call(Runtime::LocalEnvironment &env);

		std::shared_ptr<Runtime::GlobalEnvironment> _genv;
	};
}
