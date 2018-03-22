#pragma once
#include "../prilib/include/lightlist.h"
#include "config.h"
#include "typeinfo.h"

namespace CVM
{
	struct FunctionInfo
	{
	public:
		using ArgList = PriLib::lightlist<Config::RegisterIndexType>;
		using TypeList = PriLib::lightlist<TypeIndex>;

	public:
		FunctionInfo() = default;
		FunctionInfo(const FunctionInfo &info) = default;

		explicit FunctionInfo(Config::RegisterIndexType &&dysize, TypeList &&stvarb_typelist, ArgList &&arglist) :
			_dyvarb_count(std::move(dysize)),
			stvarb_typelist(std::move(stvarb_typelist)),
			arglist(std::move(arglist)) {}

		explicit FunctionInfo(FunctionInfo &&info) :
			_dyvarb_count(std::move(info._dyvarb_count)),
			stvarb_typelist(std::move(info.stvarb_typelist)),
			arglist(std::move(info.arglist)) {}

	public:
		Config::RegisterIndexType _dyvarb_count = 0;
		TypeList stvarb_typelist;
		ArgList arglist;

	public:
		bool is_dyvarb(Config::RegisterIndexType index) const {
			return Config::is_dynamic(index, dyvarb_count(), stvarb_count());
		}
		bool is_stvarb(Config::RegisterIndexType index) const {
			return Config::is_static(index, dyvarb_count(), stvarb_count());
		}
		TypeIndex get_stvarb_type(Config::RegisterIndexType index) const {
			return stvarb_typelist.at(Config::get_static_id(index, dyvarb_count(), stvarb_count()));
		}

		Config::RegisterIndexType dyvarb_count() const {
			return _dyvarb_count;
		}
		Config::RegisterIndexType stvarb_count() const {
			return Config::convertToRegisterIndexType(stvarb_typelist.size());
		}
		Config::RegisterIndexType regsize() const {
			return dyvarb_count() + stvarb_count();
		}
	};
}
