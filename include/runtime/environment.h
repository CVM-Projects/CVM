#pragma once
#include "typeinfo.h"
#include "registerset.h"
#include "controlflow.h"
#include "datapool.h"
#include "functable.h"
#include <set>
#include <list>
#include <memory>

namespace CVM
{
	class VirtualMachine;

	namespace Runtime
	{
		// This is different from Instruction::EnvType.
		enum EnvType {
			e_current = 0,  // %env
			e_parent = 1,   // %penv
			e_temp = 2,     // %tenv
		};

		class Environment;

		class EnvironmentSet
		{
		public:
			EnvironmentSet() = default;

			void add(const std::shared_ptr<Environment> &env) {
				_data.push_back(env);
			}
			void remove(Environment *envp) {
				assert(_data.back().get() == envp);
				_data.pop_back();
			}

		private:
			std::list<std::shared_ptr<Environment>> _data;
		};

		class GlobalEnvironment;

		class Environment
		{
		public:
			explicit Environment(const DataRegisterSet &drs)
				: _dataRegisterSet(drs) {}

			virtual ~Environment() {}

			virtual void addSubEnvironment(Environment *envp) {
				envp->SetGEnv(_genv);
				envp->SetPEnv(this);
				std::shared_ptr<Environment> env(envp);
				_subenv_set.add(env);
			}

			void removeSubEnvironment(Environment *envp) {
				if (envp) {
					_subenv_set.remove(envp);
				}
			}

			virtual bool isLocal() const {
				return false;
			}

			const TypeInfo& getType(TypeIndex index) const {
				return getTypeInfoMap().at(index);
			}
			virtual DataRegisterSet& getDataRegisterSet(EnvType etype) {
				if (etype == e_current) {
					return getDataRegisterSet();
				}
				else if (etype == e_parent) {
					return _penv->getDataRegisterSet();
				}
				else if (etype == e_temp) {
					return _tenv->getDataRegisterSet();
				}
				else {
					assert(false);
					return getDataRegisterSet();
				}
			}
			bool is_dyvarb(Config::RegisterIndexType index, EnvType etype) { // TODO : Will be removed.
				return getDataRegisterSet(etype).is_dynamic(index);
			}
			bool is_stvarb(Config::RegisterIndexType index, EnvType etype) { // TODO : Will be removed.
				return getDataRegisterSet(etype).is_static(index);
			}
			bool is_dyvarb(Config::RegisterIndexType index) {
				return getDataRegisterSet().is_dynamic(index);
			}
			bool is_stvarb(Config::RegisterIndexType index) {
				return getDataRegisterSet().is_static(index);
			}
			DataRegisterDynamic& get_dyvarb(Config::RegisterIndexType index) {
				return getDataRegisterSet().get_dynamic(index);
			}
			DataRegisterStatic& get_stvarb(Config::RegisterIndexType index) {
				return getDataRegisterSet().get_static(index);
			}
			DataRegisterDynamic& get_dyvarb(Config::RegisterIndexType index, EnvType etype) {
				return getDataRegisterSet(etype).get_dynamic(index);
			}
			DataRegisterStatic& get_stvarb(Config::RegisterIndexType index, EnvType etype) {
				return getDataRegisterSet(etype).get_static(index);
			}
			DataRegisterSet& getDataRegisterSet() {
				return _dataRegisterSet;
			}
			const ResultRegister& get_result() const {
				return _resultRegister;
			}
			ResultRegister& get_result() {
				return _resultRegister;
			}

			GlobalEnvironment& GEnv() const {
				return *_genv;
			}
			Environment& PEnv() const {
				return *_penv;
			}
			void SetPEnv(Environment *penv) {
				assert(_penv == nullptr);
				_penv = penv;
			}
			void SetGEnv(GlobalEnvironment *genv) {
				assert(_genv == nullptr);
				_genv = genv;
			}
			virtual const TypeInfoMap& getTypeInfoMap() const;

		protected:
			GlobalEnvironment *_genv = nullptr;
			Environment *_penv = nullptr;
			Environment *_tenv = nullptr;
			DataRegisterSet _dataRegisterSet;
			EnvironmentSet _subenv_set;
			ResultRegister _resultRegister;
		};

		class GlobalEnvironment : public Environment
		{
		public:
			using DataSectionMap = LiteralDataPool;
			explicit GlobalEnvironment(
				const DataRegisterSet &drs,
				const TypeInfoMap *tim,
				const DataSectionMap *datasmap,
				const FuncTable *functable,
				HashStringPool *_hashStringPool
			)
				: Environment(drs), _tim(tim), _datasmap(datasmap), _functable(functable), _hashStringPool(_hashStringPool) {}

			GlobalEnvironment(const GlobalEnvironment &) = delete;

			~GlobalEnvironment() {
				// TODO
				delete _functable;
				delete _datasmap;
				delete _tim;
			}

			virtual void addSubEnvironment(Environment *envp) {
				Environment::addSubEnvironment(envp);
				envp->SetGEnv(this);
			}
			const DataSectionMap& getDataSectionMap() const {
				return *_datasmap;
			}
			const FuncTable& getFuncTable() const {
				return *_functable;
			}
			void setVM(VirtualMachine *vmp) {
				_vmp = vmp;
			}
			VirtualMachine& getVM() {
				return *_vmp;
			}
			const TypeInfoMap& getTypeInfoMap() const {
				return *_tim;
			}
			const std::string& getHashIDContext(const HashID &hashID) {
				return _hashStringPool->get(hashID);
			}

		private:
			// TODO : Change const * to std::shared_ptr
			const TypeInfoMap *_tim;
			const DataSectionMap *_datasmap;
			const FuncTable *_functable;
			VirtualMachine *_vmp;
			LCMM::MemoryManager _memory_manager;
			HashStringPool *_hashStringPool;  // TODO: Make it not a pointer.
		};

		inline const TypeInfoMap& Environment::getTypeInfoMap() const {
			return GEnv().getTypeInfoMap();
		}

		class ThreadEnvironment : public Environment
		{
		public:
			explicit ThreadEnvironment(const DataRegisterSet &drs)
				: Environment(drs) {}
		};

		class LocalEnvironment : public Environment
		{
		public:
			explicit LocalEnvironment(const DataRegisterSet &drs, const InstFunction &func)
				: Environment(drs), _func(func), _controlflow(_func) {}

			ControlFlow& Controlflow() {
				return _controlflow;
			}

			bool isLocal() const {
				return true;
			}

			InstFunction _func;
			ControlFlow _controlflow;
		};
	}
}
