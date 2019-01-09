#pragma once
#include "runtime/instruction.h"
#include "config.h"
#include "runtime/datamanage.h"
#include "inststruct/instpart.h"

#define CVMInstDebugMode false

namespace CVM
{
	namespace Runtime
	{
		inline static void CheckLocalEnv(Runtime::Environment &env) {
			if (!env.isLocal()) {
				puts("Error Occur because of not LocalEnvironment.");
				exit(-1);
			}
		}

		namespace Insts
		{
			//--------------------------------------
			// * Nope
			//--------------------------------------

			struct Nope : public Instruction {
				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <Nope>");
				}
			};
		}

		namespace Insts
		{
			//--------------------------------------
			// * Move
			//--------------------------------------

			struct MoveRegisterDD : public Instruction {
				Config::RegisterIndexType dst;
				Config::RegisterIndexType src;

				MoveRegisterDD(Config::RegisterIndexType dst, Config::RegisterIndexType src)
					: dst(dst), src(src) {}
			};

			struct MoveRegisterDdDd : public MoveRegisterDD {
				MoveRegisterDdDd(Config::RegisterIndexType dst, Config::RegisterIndexType src)
					: MoveRegisterDD(dst, src) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDdDd> ", to_string(dst), " -> ", to_string(src));
					DataManage::MoveRegisterDdDd(env, env.get_dyvarb(dst), env.get_dyvarb(src));
				}
			};
			struct MoveRegisterDsDd : public MoveRegisterDD {
				MoveRegisterDsDd(Config::RegisterIndexType dst, Config::RegisterIndexType src)
					: MoveRegisterDD(dst, src) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDsDd> ", to_string(dst), " -> ", to_string(src));
					DataManage::MoveRegisterDsDd(env, env.get_stvarb(dst), env.get_dyvarb(src));
				}
			};
			struct MoveRegisterDdDs : public MoveRegisterDD {
				TypeIndex srctype;

				MoveRegisterDdDs(Config::RegisterIndexType dst, Config::RegisterIndexType src, TypeIndex srctype)
					: MoveRegisterDD(dst, src), srctype(srctype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDdDs> ", to_string(dst), " -> ", to_string(src));
					DataManage::MoveRegisterDdDs(env, env.get_dyvarb(dst), env.get_stvarb(src), srctype);
				}
			};
			struct MoveRegisterDsDs : public MoveRegisterDD {
				TypeIndex srctype;

				MoveRegisterDsDs(Config::RegisterIndexType dst, Config::RegisterIndexType src, TypeIndex srctype)
					: MoveRegisterDD(dst, src), srctype(srctype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDsDs> ", to_string(dst), " -> ", to_string(src));
					DataManage::MoveRegisterDsDs(env, env.get_stvarb(dst), env.get_stvarb(src), srctype);
				}
			};

			struct MoveRegisterResD : public Instruction {
				Config::RegisterIndexType src;

				MoveRegisterResD(Config::RegisterIndexType src)
					: src(src) {}
			};

			struct MoveRegisterResDd : public MoveRegisterResD {
				MoveRegisterResDd(Config::RegisterIndexType src)
					: MoveRegisterResD(src) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterResDd> ", to_string(src));
					DataManage::MoveRegisterResDd(env, env.get_dyvarb(src));
				}
			};
			struct MoveRegisterResDs : public MoveRegisterResD {
				TypeIndex srctype;

				MoveRegisterResDs(Config::RegisterIndexType src, TypeIndex srctype)
					: MoveRegisterResD(src), srctype(srctype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterResDs> ", to_string(src));
					DataManage::MoveRegisterResDs(env, env.get_stvarb(src), srctype);
				}
			};

			struct MoveRegisterDRes : public Instruction {
				Config::RegisterIndexType dst;
				TypeIndex restype;

				MoveRegisterDRes(Config::RegisterIndexType dst, TypeIndex restype)
					: dst(dst), restype(restype) {}
			};
			struct MoveRegisterDdRes : public MoveRegisterDRes {
				MoveRegisterDdRes(Config::RegisterIndexType dst, TypeIndex restype)
					: MoveRegisterDRes(dst, restype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDdRes> ", to_string(dst));
					DataManage::MoveRegisterDdRes(env, env.get_dyvarb(dst), restype);
				}
			};
			struct MoveRegisterDsRes : public MoveRegisterDRes {
				MoveRegisterDsRes(Config::RegisterIndexType dst, TypeIndex restype)
					: MoveRegisterDRes(dst, restype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <MoveRegisterDsRes> ", to_string(dst));
					DataManage::MoveRegisterDsRes(env, env.get_stvarb(dst), restype);
				}
			};
		}

		namespace Insts
		{
			//--------------------------------------
			// * LoadData
			//--------------------------------------

			template <size_t _subid>
			struct LoadData;
			
			template <>
			struct LoadData<1> : public Instruction {
				using DataType = InstStruct::Data::Type;
				TypeIndex dsttype;
				DataType data;

				LoadData(TypeIndex dsttype, DataType data)
					: dsttype(dsttype), data(data) {}

			protected:
				ConstDataPointer get_datapointer(Environment &env) const {
					return ConstDataPointer(&data);
				}
				MemorySize get_memorysize(Environment &env) const {
					return MemorySize(sizeof(DataType));
				}
			};
			template <>
			struct LoadData<2> : public Instruction {
				using DataType = InstStruct::DataIndex::Type;
				TypeIndex dsttype;
				DataType data;

				LoadData(TypeIndex dsttype, DataType data)
					: dsttype(dsttype), data(data) {}

			protected:
				ConstDataPointer get_datapointer(Environment &env) const {
					return ConstDataPointer(env.GEnv().getDataSectionMap().at(data).first);
				}
				MemorySize get_memorysize(Environment &env) const {
					return MemorySize(env.GEnv().getDataSectionMap().at(data).second);
				}
			};

			template <size_t _subid>
			struct LoadDataDd : public LoadData<_subid> {
				Config::RegisterIndexType dst;

				LoadDataDd(Config::RegisterIndexType dst, TypeIndex expecttype, typename LoadData<_subid>::DataType data)
					: LoadData<_subid>(expecttype, data), dst(dst) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataDd", _subid, ">");
					DataManage::LoadDataDd(env, env.get_dyvarb(dst), LoadData<_subid>::dsttype, LoadData<_subid>::get_datapointer(env), LoadData<_subid>::get_memorysize(env));
				}
			};
			template <size_t _subid>
			struct LoadDataDs : public LoadData<_subid> {
				Config::RegisterIndexType dst;

				LoadDataDs(Config::RegisterIndexType dst, TypeIndex dsttype, typename LoadData<_subid>::DataType data)
					: LoadData<_subid>(dsttype, data), dst(dst) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataDs", _subid, ">");
					DataManage::LoadDataDs(env, env.get_stvarb(dst), LoadData<_subid>::dsttype, LoadData<_subid>::get_datapointer(env), LoadData<_subid>::get_memorysize(env));
				}
			};
			template <size_t _subid>
			struct LoadDataRes : public LoadData<_subid> {
				LoadDataRes(TypeIndex restype, typename LoadData<_subid>::DataType data)
					: LoadData<_subid>(restype, data) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataRes", _subid, ">");
					DataManage::LoadDataRes(env, LoadData<_subid>::dsttype, LoadData<_subid>::get_datapointer(env), LoadData<_subid>::get_memorysize(env));
				}
			};

			//--------------------------------------
			// * LoadDataPointer
			//--------------------------------------

			struct LoadDataPointer : public Instruction {
				using DataType = InstStruct::DataIndex::Type;
				DataType data;

				LoadDataPointer(DataType data)
					: data(data) {}

				ConstDataPointer get_datapointer(Environment &env) const {
					auto dp = env.GEnv().getDataSectionMap().at(data).first;
					return ConstDataPointer(dp);
				}
			};
			struct LoadDataPointerDd : public LoadDataPointer {
				Config::RegisterIndexType dst;

				LoadDataPointerDd(Config::RegisterIndexType dst, DataType data)
					: LoadDataPointer(data), dst(dst) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataPointerDd>");
					DataManage::LoadDataPointerDd(env, env.get_dyvarb(dst), get_datapointer(env));
				}
			};
			struct LoadDataPointerDs : public LoadDataPointer {
				Config::RegisterIndexType dst;
				TypeIndex dsttype;

				LoadDataPointerDs(Config::RegisterIndexType dst, TypeIndex dsttype, DataType data)
					: LoadDataPointer(data), dst(dst), dsttype(dsttype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataPointerDs>");
					DataManage::LoadDataPointerDs(env, env.get_stvarb(dst), dsttype, get_datapointer(env));
				}
			};
			struct LoadDataPointerRes : public LoadDataPointer {
				TypeIndex restype;

				LoadDataPointerRes(TypeIndex restype, DataType data)
					: LoadDataPointer(data), restype(restype) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <LoadDataPointerRes>");
					DataManage::LoadDataPointerRes(env, restype, get_datapointer(env));
				}
			};
		}

		namespace Insts
		{
			//--------------------------------------
			// * Jump
			//--------------------------------------

			struct Jump : public Instruction {
				Config::LineCountType line;

				Jump(Config::LineCountType line)
					: line(line) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <Jump>");
					CheckLocalEnv(env);
					static_cast<Runtime::LocalEnvironment&>(env).Controlflow().setProgramCounter(line);
				}
			};
		}

		namespace Insts
		{
			//--------------------------------------
			// * Call
			//--------------------------------------

			struct Call : public Instruction {
				using ArgListType = PriLib::lightlist<Config::RegisterIndexType>;
				Config::FuncIndexType fid;
				ArgListType arglist;

				Call(Config::FuncIndexType fid, ArgListType arglist)
					: fid(fid), arglist(arglist) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <Call> ", env.GEnv().getFuncTable().at(fid)->type(), ":", fid);
					DataManage::CallZero(env, fid, arglist);
				}
			};

			struct CallDds : public Call {
				Config::RegisterIndexType dst;

				CallDds(Config::RegisterIndexType dst, Config::FuncIndexType fid, ArgListType arglist)
					: Call(fid, arglist), dst(dst) {
					assert(dst);
				}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <CallDds> ", env.GEnv().getFuncTable().at(fid)->type(), ":", fid);
					DataManage::CallDds(env, dst, fid, arglist);
				}
			};
			struct CallRes : public Call {
				CallRes(Config::FuncIndexType fid, ArgListType arglist)
					: Call(fid, arglist) {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <CallRes> ", env.GEnv().getFuncTable().at(fid)->type(), ":", fid);
					DataManage::CallRes(env, fid, arglist);
				}
			};

			//--------------------------------------
			// * Return
			//--------------------------------------

			struct Return : public Instruction {
				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <Return>");
					CheckLocalEnv(env);
					static_cast<Runtime::LocalEnvironment&>(env).Controlflow().setProgramCounterEnd();
				}
			};
		}

		namespace InstsDebug
		{
			//--------------------------------------
			// * Debug
			//--------------------------------------

			struct OutputRegister : public Instruction {
				OutputRegister() {}

				virtual void operator()(Environment &env) const {
					if (CVMInstDebugMode)
						println("Do Inst <Debug:OutputRegister>");
					CheckLocalEnv(env);
					const auto &typelist = ((Runtime::LocalEnvironment&)(env))._func.info().sttypelist();
					auto typelist_count = env.getDataRegisterSet().stsize();
					Config::RegisterIndexType regcount = 0;
					auto &regset = env.getDataRegisterSet();
					PriLib::Output::println("=======================");
					PriLib::Output::println("dyvarb:");
					for (Config::RegisterIndexType i = 1; i <= regset.dysize(); i++) {
						PriLib::Output::print("  %", ++regcount, ": type(", env.GEnv().getHashIDContext(env.getType(regset.get_dynamic(i).type).name.data), "), ");
						Runtime::DataManage::Debug_PrintRegister(env, regset.get_dynamic(i));
					}
					if (regset.stsize() != 0) {
						PriLib::Output::print("stvarb:");
						//for (size_t i = regset.dysize(); i <= regset.stsize() + regset.stsize());
						//env.getType()
						Runtime::DataPointer address = regset.get_static(regset.dysize() + 1).data;
						printf(" [address : 0x%p]\n", address.get());
						for (Config::RegisterIndexType i = 0; i < typelist_count; ++i) {
							MemorySize size = env.getType(typelist[i]).size;
							PriLib::Output::print("  %", ++regcount, ": type(", env.GEnv().getHashIDContext(env.getType(typelist[i]).name.data), "), ");
							const auto &str = Runtime::DataManage::ToStringData(address, size);
							PriLib::Output::println(str);
							address = address.offset(size);
						}
					}
					PriLib::Output::println("=======================");
					PriLib::Output::println();
				}
			};
		}
	}
}
