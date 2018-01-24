#include "basic.h"
#include "runtime/registerset.h"

namespace CVM
{
	namespace Runtime
	{
		void DataRegisterSetDynamic::initialize() {
			auto diter = _data.begin();
			while (diter != _data.end()) {
				*diter = DataRegisterDynamic();
				++diter;
			}
		}

		void DataRegisterSetStatic::initialize(DataPointer address, const SizeList& sizelist) {
			assert(size() == sizelist.size());
			auto diter = _data.begin();
			auto siter = sizelist.begin();
			DataPointer start = address;
			while (diter != _data.end()) {
				*diter = DataRegisterStatic(address);
				address = address.offset(siter->data);
				++diter;
				++siter;
			}
			auto offset = address.get<byte>() - start.get<byte>();
			assert(offset <= UINT32_MAX);
			_memsize = MemorySize(static_cast<uint32_t>(offset));
		}
	}
}
