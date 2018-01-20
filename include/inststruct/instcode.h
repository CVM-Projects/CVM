#pragma once
#include <cstdint>

namespace CVM
{
	namespace InstStruct
	{
		enum InstCode : uint8_t {
			i_nop = 0,
			i_mov,
			i_load,
			i_loadt,
			i_tset,
			i_cpyn,
			i_ret,

			id_opreg,
		};
	}
}
