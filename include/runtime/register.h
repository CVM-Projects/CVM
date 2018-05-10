#pragma once
#include "datapointer.h"
#include "typeinfo.h"
#include "../lcmm/include/lcmm.h"

namespace CVM
{
	namespace Runtime
	{
		enum DataRegisterType
		{
			rt_null,
			rt_dynamic,
			rt_static,
		};

		struct DataRegister {};

		struct DataRegisterStatic : public DataRegister
		{
			explicit DataRegisterStatic() = default;

			explicit DataRegisterStatic(const DataPointer &dp)
				: data(dp) {}

			DataPointer data;
		};

		struct DataRegisterDynamic : public DataRegister, public LCMM::Object
		{
			explicit DataRegisterDynamic() = default;

			explicit DataRegisterDynamic(const TypeIndex &ti, const DataPointer &dp)
				: type(ti), data(dp) {}

			TypeIndex type;
			DataPointer data;
		};

		struct ResultRegister
		{
			explicit ResultRegister() = default;

			explicit ResultRegister(DataRegisterType rtype, DataRegister *drp)
				: rtype(rtype), drp(drp) {}

			void set(DataRegisterDynamic &drd) {
				rtype = rt_dynamic;
				drp = &drd;
			}
			void set(DataRegisterStatic &drs) {
				rtype = rt_static;
				drp = &drs;
			}
			
			DataRegisterType rtype = rt_null;
			DataRegister *drp = nullptr;
		};

		class Environment;

		struct EnvRegister
		{
			Environment* env = nullptr;
		};
	}
}
