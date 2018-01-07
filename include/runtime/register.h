#pragma once
#include "datapointer.h"
#include "typeinfo.h"

namespace CVM
{
	namespace Runtime
	{
		struct DataRegisterStatic
		{
			DataRegisterStatic() = default;

			explicit DataRegisterStatic(const DataPointer &dp)
				: data(dp) {}

			DataPointer data;
		};

		struct DataRegisterDynamic
		{
			DataRegisterDynamic() = default;

			explicit DataRegisterDynamic(const TypeIndex &ti, const DataPointer &dp)
				: type(ti), data(dp) {}

			TypeIndex type;
			DataPointer data;
		};

		class Environment;

		struct EnvRegister
		{
			Environment* env = nullptr;
		};
	}
}
