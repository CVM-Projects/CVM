#pragma once
#include "functioninfo.h"
#include "config.h"

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

			bool is_dyvarb(Config::RegisterIndexType index) const {
				return Config::is_dynamic(index, dyvarb_count(), stvarb_count());
			}
			bool is_stvarb(Config::RegisterIndexType index) const {
				return Config::is_static(index, dyvarb_count(), stvarb_count());
			}
			TypeIndex get_stvarb_type(Config::RegisterIndexType index) const {
				return _info.stvarb_typelist.at(Config::get_static_id(index, dyvarb_count(), stvarb_count()));
			}
			const TypeList& stvarb_typelist() const {
				return _info.stvarb_typelist;
			}
			const ArgList& arglist() const {
				return _info.arglist;
			}

			Config::RegisterIndexType dyvarb_count() const {
				return _info.dyvarb_count;
			}
			Config::RegisterIndexType stvarb_count() const {
				return Config::convertToRegisterIndexType(_info.stvarb_typelist.size());
			}

		private:
			FunctionInfo _info;
		};
	}
}
