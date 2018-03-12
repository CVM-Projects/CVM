#pragma once
#include "basic.h"
#include <vector>
#include "instruction.h"
#include "datapointer.h"

namespace CVM
{
	namespace Runtime
	{
		enum FunctionType
		{
			ft_null,
			ft_inst,
			ft_ptr,
		};

		class Function
		{
		public:
			virtual FunctionType type() const {
				return ft_null;
			}
		};

		class InstFunction : public Function
		{
		public:
			using InstList = std::vector<Instruction>;
		public:
			explicit InstFunction(const InstList &il)
				: _data(il) {}

			virtual FunctionType type() const {
				return ft_inst;
			}

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

		class PointerFunction : public Function
		{
		public:
			using Result = DataPointer;
			using ArgumentList = PriLib::lightlist<DataPointer>;
			using Func = void(Result &, ArgumentList &);
		public:
			explicit PointerFunction(std::function<Func> func)
				: _data(func) {}

			virtual FunctionType type() const {
				return ft_ptr;
			}

			const std::function<Func>& data() const {
				return _data;
			}

		private:
			std::function<Func> _data;
		};
	}
}
