#pragma once
#include "runtime/environment.h"

namespace CVM
{
	class VirtualMachine
	{
	public:
		VirtualMachine() {}

		void addGlobalEnvironment(Runtime::GlobalEnvironment *genv) {
			_genv = std::shared_ptr<Runtime::GlobalEnvironment>(genv);
			_genv->setVM(this);
		}

		Runtime::GlobalEnvironment& Genv() {
			return *_genv;
		}

		void Call(Runtime::LocalEnvironment &env);

		std::shared_ptr<Runtime::GlobalEnvironment> _genv;
	};
}
