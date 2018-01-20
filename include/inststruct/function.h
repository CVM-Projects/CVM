#pragma once
#include "functioninfo.h"

namespace CVM
{
	namespace InstStruct
	{
		class Function
		{
		public:
			explicit Function(FunctionInfo &&info)
				: _info(std::move(info)) {}

			const InstList& instlist() const {
				return _info.instdata;
			}

			bool is_dyvarb(size_t index) const {
				return index <= dyvarb_count();
			}
			bool is_stvarb(size_t index) const {
				return dyvarb_count() < index && index <= dyvarb_count() + stvarb_count();
			}
			TypeIndex get_stvarb_type(size_t index) const {
				return _info.stvarb_typelist.at(index - _info.dyvarb_count - 1);
			}
			const TypeList& stvarb_typelist() const {
				return _info.stvarb_typelist;
			}

			size_t dyvarb_count() const {
				return _info.dyvarb_count;
			}
			size_t stvarb_count() const {
				return _info.stvarb_typelist.size();
			}

		private:
			FunctionInfo _info;
		};
	}
}
