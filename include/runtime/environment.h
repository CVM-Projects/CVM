#pragma once
#include "typeinfo.h"
#include "registerset.h"
#include "controlflow.h"
#include "datapool.h"
#include <set>
#include <list>
#include <memory>

namespace CVM
{
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
				envp->SetPEnv(this);
				std::shared_ptr<Environment> env(envp);
				_subenv_set.add(env);
			}

			virtual bool isLocal() const {
				return false;
			}

			const TypeInfo& getType(TypeIndex index) const {
				return _timp->at(index);
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
			DataRegisterDynamic& get_dyvarb(Config::RegisterIndexType index, EnvType etype) {
				return getDataRegisterSet(etype).get_dynamic(index);
			}
			DataRegisterStatic& get_stvarb(Config::RegisterIndexType index, EnvType etype) {
				return getDataRegisterSet(etype).get_static(index);
			}
			DataRegisterSet& getDataRegisterSet() {
				return _dataRegisterSet;
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
			void SetTypeInfoMap(TypeInfoMap *timp) {
				_timp = timp;
			}

		protected:
			GlobalEnvironment *_genv = nullptr;
			Environment *_penv = nullptr;
			Environment *_tenv = nullptr;
			TypeInfoMap *_timp = nullptr;
			DataRegisterSet _dataRegisterSet;
			EnvironmentSet _subenv_set;
		};

		class GlobalEnvironment : public Environment
		{
		public:
			using DataSectionMap = LiteralDataPool;
			explicit GlobalEnvironment(const DataRegisterSet &drs, const TypeInfoMap &tim, const DataSectionMap &datasmap)
				: Environment(drs), _tim(tim), _datasmap(datasmap) {
				_timp = &_tim;
			}

			GlobalEnvironment(const GlobalEnvironment &) = delete;

			virtual void addSubEnvironment(Environment *envp) {
				Environment::addSubEnvironment(envp);
				envp->SetTypeInfoMap(this->_timp);
				envp->SetGEnv(this);
			}
			const DataSectionMap& getDataSectionMap() const {
				return _datasmap;
			}

		private:
			TypeInfoMap _tim;
			DataSectionMap _datasmap;
		};

		class ThreadEnvironment : public Environment
		{
		public:
			explicit ThreadEnvironment(const DataRegisterSet &drs)
				: Environment(drs) {}
		};

		class LocalEnvironment : public Environment
		{
		public:
			explicit LocalEnvironment(const DataRegisterSet &drs, const Function &func)
				: Environment(drs), _func(func), _controlflow(_func) {}

			ControlFlow& Controlflow() {
				return _controlflow;
			}

			bool isLocal() const {
				return true;
			}

			Function _func;
			ControlFlow _controlflow;
		};
	}
}
