#pragma once
#include <vector>
#include <memory>
#include "typeinfo.h"
#include "instruction.h"
#include "instpart.h"

namespace CVM
{
	namespace InstStruct
	{
		using InstList = std::vector<Instruction*>; // TODO
		using TypeList = std::vector<TypeIndex>;
		using ArgList = std::vector<Config::RegisterIndexType>;

		struct FunctionInfo
		{
			explicit FunctionInfo() = default;

			explicit FunctionInfo(FunctionInfo &&info) :
				instdata(std::move(info.instdata)),
				stvarb_typelist(std::move(info.stvarb_typelist)),
				arglist(std::move(info.arglist)),
				dyvarb_count(std::move(info.dyvarb_count)) {}

			explicit FunctionInfo(InstList &&il, Config::RegisterIndexType &&dysize, TypeList &&stvarb_typelist, ArgList &&arglist) :
				instdata(std::move(il)),
				dyvarb_count(std::move(dysize)),
				stvarb_typelist(std::move(stvarb_typelist)),
				arglist(std::move(arglist)) {}
			
			Config::RegisterIndexType regsize() const {
				return dyvarb_count + Config::convertToRegisterIndexType(stvarb_typelist.size());
			}

			InstList instdata;
			TypeList stvarb_typelist;
			ArgList arglist;
			Config::RegisterIndexType dyvarb_count = 0;
		};
	}
}
