#pragma once
#include <vector>
#include <memory>
#include "typeinfo.h"
#include "instruction.h"

namespace CVM
{
	namespace Instruction
	{
		class Function
		{
		public:
			using InstList = std::vector<Instruction*>; // TODO
			using TypeList = std::vector<TypeIndex>;
			using ArgList = std::vector<bool>; // true : dyvarb, false : stvarb
		public:
			explicit Function(const InstList &il, size_t dysize, const TypeList &stvarb_typelist, const ArgList &arglist)
				: _data(il), _dyvarb_count(dysize), _stvarb_typelist(stvarb_typelist), _arglist(arglist) {}

			const InstList& instlist() const {
				return _data;
			}

			bool is_dyvarb(size_t index) const {
				return index <= dyvarb_count();
			}
			bool is_stvarb(size_t index) const {
				return dyvarb_count() < index && index <= dyvarb_count() + stvarb_count();
			}
			TypeIndex get_stvarb_type(size_t index) const {
				return _stvarb_typelist.at(index - _dyvarb_count - 1);
			}
			const TypeList& stvarb_typelist() const {
				return _stvarb_typelist;
			}

			size_t dyvarb_count() const {
				return _dyvarb_count;
			}
			size_t stvarb_count() const {
				return _stvarb_typelist.size();
			}

		private:
			InstList _data;
			TypeList _stvarb_typelist;
			ArgList _arglist;
			size_t _dyvarb_count = 0;
		};
	}
}
