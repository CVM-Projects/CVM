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
			i_loadp,
			i_tset,
			i_cpyn,
			i_call,
			i_ret,

			i_jump,

			id_opreg,
		};
	}
}
