#pragma once
#include "instruction.h"
#include "instpart.h"

namespace CVM
{
	namespace InstStruct
	{
		namespace Insts
		{
			template <InstCode _ic>
			struct Inst : public Instruction
			{
				Inst() : Instruction(_ic) {}
			};

			template <InstCode _ic, uint8_t _id>
			struct Instx : public Instruction
			{
				Instx() : Instruction(_ic, _id) {}
			};

			//=======================================

			struct Move : public Inst<i_mov>
			{
				explicit Move(const Register &dst, const Register &src)
					: dst(dst), src(src) {}

				Register dst;
				Register src;
			};

			struct Load1 : public Instx<i_load, 1>
			{
				explicit Load1(const Register &dst, const Data &src, const TypeIndex &type)
					: dst(dst), src(src), type(type) {}

				Register dst;
				Data src;
				TypeIndex type;
			};
			struct Load2 : public Instx<i_load, 2>
			{
				explicit Load2(const Register &dst, const DataIndex &src, const TypeIndex &type)
					: dst(dst), src(src), type(type) {}

				Register dst;
				DataIndex src;
				TypeIndex type;
			};

			struct LoadPointer1 : public Instx<i_loadp, 1>
			{
				explicit LoadPointer1(const Register &dst, const Data &src)
					: dst(dst), src(src) {}

				Register dst;
				Data src;
			};
			struct LoadPointer2 : public Instx<i_loadp, 2>
			{
				explicit LoadPointer2(const Register &dst, const DataIndex &src)
					: dst(dst), src(src) {}

				Register dst;
				DataIndex src;
			};

			struct Call : public Inst<i_call>
			{
				explicit Call(const Register &dst, const Identifier &func, const ArgumentList &arglist)
					: dst(dst), func(func), arglist(arglist) {}

				Register dst;
				Identifier func;
				ArgumentList arglist;
			};

			struct Return : public Inst<i_ret> {};
			struct Nope : public Inst<i_nop> {};

			struct Debug_OutputRegister : public Inst<id_opreg> {};
		}
	}
}
