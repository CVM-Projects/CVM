#pragma once
#include <cassert>
#include <vector>
#include "../prilib/include/lightlist.h"
#include "register.h"
#include "typeinfo.h"
#include "config.h"

namespace CVM
{
	namespace Runtime
	{
		// The Base DataRegisterSet Class
		template <typename DataRegister>
		class DataRegisterSetBase
		{
		public:
			explicit DataRegisterSetBase()
				: _size(0) {}

			explicit DataRegisterSetBase(Config::RegisterIndexType size)
				: _size(size), _data(size) {}

			DataRegister& get(Config::RegisterIndexType id) {
				return _data.at(id);
			}
			const DataRegister& get(Config::RegisterIndexType id) const {
				return _data.at(id);
			}

			Config::RegisterIndexType size() const {
				return _size;
			}

		protected:
			Config::RegisterIndexType _size;
			PriLib::lightlist<DataRegister> _data;
		};

		// The DataRegisterSet (Dynamic) Class
		class DataRegisterSetDynamic : public DataRegisterSetBase<DataRegisterDynamic>
		{
		public:
			explicit DataRegisterSetDynamic()
				: DataRegisterSetBase() {}

			explicit DataRegisterSetDynamic(Config::RegisterIndexType size)
				: DataRegisterSetBase(size) {
				initialize();
			}

		private:
			void initialize();
		};

		// The DataRegisterSet (Static) Class
		class DataRegisterSetStatic : public DataRegisterSetBase<DataRegisterStatic>
		{
		public:
			using SizeList = PriLib::lightlist<MemorySize>;

		public:
			explicit DataRegisterSetStatic()
				: DataRegisterSetBase() {}

			explicit DataRegisterSetStatic(Config::RegisterIndexType size, DataPointer address, const SizeList &sizelist)
				: DataRegisterSetBase(size) {
				initialize(address, sizelist);
			}
			MemorySize memsize() const {
				return _memsize;
			}

		private:
			void initialize(DataPointer address, const SizeList &sizelist);
			MemorySize _memsize;
		};

		// The DataRegisterSet (Whole) Class
		class DataRegisterSet
		{
		public:
			DefineExplicitType(DyDatRegSize, Config::RegisterIndexType);
			DefineExplicitType(StDatRegSize, Config::RegisterIndexType);

		public:
			explicit DataRegisterSet(DyDatRegSize dy_size)
				: _dynamic(dy_size.data) {}

			explicit DataRegisterSet(StDatRegSize st_size, DataPointer address, const DataRegisterSetStatic::SizeList &sizelist)
				: _static(st_size.data, address, sizelist) {}

			explicit DataRegisterSet(DyDatRegSize dy_size, StDatRegSize st_size, DataPointer address, const DataRegisterSetStatic::SizeList &sizelist)
				: _dynamic(dy_size.data), _static(st_size.data, address, sizelist) {}

			bool is_dynamic(Config::RegisterIndexType id) {
				return Config::is_dynamic(id, dysize(), stsize());
			}
			bool is_static(Config::RegisterIndexType id) {
				return Config::is_static(id, dysize(), stsize());
			}
			DataRegisterDynamic& get_dynamic(Config::RegisterIndexType id) {
				assert(is_dynamic(id));
				return _dynamic.get(Config::get_dynamic_id(id, dysize(), stsize()));
			}
			DataRegisterStatic& get_static(Config::RegisterIndexType id) {
				assert(is_static(id));
				return _static.get(Config::get_static_id(id, dysize(), stsize()));
			}

			Config::RegisterIndexType dysize() const {
				return _dynamic.size();
			}
			Config::RegisterIndexType stsize() const {
				return _static.size();
			}

			MemorySize stmemsize() const {
				return _static.memsize();
			}

		private:
			DataRegisterSetDynamic _dynamic;
			DataRegisterSetStatic _static;
		};
	}
}
