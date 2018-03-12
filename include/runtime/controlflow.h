#pragma once
#include "function.h"

namespace CVM
{
	namespace Runtime
	{
		class ControlFlow
		{
		public:
			ControlFlow(const InstFunction &func)
				: Func(func) {}

			void init() {
				isIncProgramCounter = true;
			}
			void callCurrInst(Environment &env) const {
				Func.inst_call(ProgramCounter, env);
			}
			bool isInstEnd() const {
				return ProgramCounter >= Func.inst_size();
			}
			bool isInstRunning() const {
				return !isInstEnd();
			}
			size_t getProgramCounter() const {
				return ProgramCounter;
			}
			void setProgramCounter(size_t count) {
				ProgramCounter = count;
				isIncProgramCounter = false;
			}
			void setProgramCounterEnd() {
				setProgramCounter(Func.inst_size());
			}
			void incProgramCounter() {
				if (isIncProgramCounter)
					++ProgramCounter;
			}

		private:
			size_t ProgramCounter = 0;
			bool isIncProgramCounter;
			const InstFunction &Func;
		};
	}
}
