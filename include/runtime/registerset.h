#pragma once
#include <cassert>
#include <vector>
#include "lightlist.h"
#include "register.h"
#include "typeinfo.h"

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

			explicit DataRegisterSetBase(size_t size)
				: _size(size), _data(size) {}

			DataRegister& get(size_t id) {
				return _data.at(id);
			}
			const DataRegister& get(size_t id) const {
				return _data.at(id);
			}

			size_t size() const {
				return _size;
			}

		protected:
			size_t _size;
			PriLib::lightlist<DataRegister> _data;
		};

		// The DataRegisterSet (Dynamic) Class
		class DataRegisterSetDynamic : public DataRegisterSetBase<DataRegisterDynamic>
		{
		public:
			explicit DataRegisterSetDynamic()
				: DataRegisterSetBase() {}

			explicit DataRegisterSetDynamic(size_t size)
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

			explicit DataRegisterSetStatic(size_t size, DataPointer address, const SizeList &sizelist)
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
			using DyDatRegSize = PriLib::ExplicitType<size_t>;
			using StDatRegSize = PriLib::ExplicitType<size_t>;

		public:
			explicit DataRegisterSet(DyDatRegSize dy_size)
				: _dynamic(dy_size.data) {}

			explicit DataRegisterSet(StDatRegSize st_size, DataPointer address, const DataRegisterSetStatic::SizeList &sizelist)
				: _static(st_size.data, address, sizelist) {}

			explicit DataRegisterSet(DyDatRegSize dy_size, StDatRegSize st_size, DataPointer address, const DataRegisterSetStatic::SizeList &sizelist)
				: _dynamic(dy_size.data), _static(st_size.data, address, sizelist) {}

			DataRegisterDynamic& get_dynamic(size_t id) {
				assert(0 < id && id <= _dynamic.size());
				return _dynamic.get(id - 1);
			}
			DataRegisterStatic& get_static(size_t id) {
				assert(_dynamic.size() < id && id <= _dynamic.size() + _static.size());
				return _static.get(id - _dynamic.size() - 1);
			}

			size_t dysize() const {
				return _dynamic.size();
			}
			size_t stsize() const {
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
