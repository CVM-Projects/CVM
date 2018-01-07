#pragma once
#include <vector>
#include "instruction.h"

namespace CVM
{
	namespace Runtime
	{
		class Function
		{
		public:
			using InstList = std::vector<Instruction>;
		public:
			explicit Function(const InstList &il)
				: _data(il) {}

			const Instruction& inst_get(size_t id) const {
				return _data.at(id);
			}

			size_t inst_size() const {
				return _data.size();
			}

			void inst_call(size_t id, Environment &env) const {
				inst_get(id)(env);
			}

			const InstList& instlist() const {
				return _data;
			}

		private:
			InstList _data;
		};
	}
}
