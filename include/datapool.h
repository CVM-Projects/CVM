#pragma once
#include <map>
#include "../prilib/include/lightlist.h"
#include "../prilib/include/memory.h"
#include "config.h"

namespace CVM
{
	using LiteralDataPoolCreater = std::map<uint32_t, std::pair<uint8_t*, uint32_t>>;

	class LiteralDataPool
	{
	public:
		explicit LiteralDataPool(const LiteralDataPoolCreater &creater) {
			uint32_t count = static_cast<uint32_t>(creater.size());
			uint32_t size = 0;
			uint32_t rsize = 0;
			for (auto &pair : creater) {
				rsize = size;
				size += pair.second.second;
				assert(rsize <= size);
			}

			_indexdata.recapacity(count);
			_data.recapacity(size);

			std::vector<std::pair<uint8_t*, uint32_t>> rdata(count);

			for (auto &pair : creater) {
				rdata[pair.first - 1] = pair.second;
			}

			uint32_t xsize = 0;
			for (size_t i = 0; i != rdata.size(); ++i) {
				_indexdata[i] = xsize;
				PriLib::Memory::copyTo(_data.get() + xsize, rdata[i].first, rdata[i].second);
				xsize += rdata[i].second;
			}
		}

		std::pair<const uint8_t*, uint32_t> at(uint32_t id) const {
			id--; // TODO
			assert(id < _indexdata.size());
			assert(_data.size() <= UINT32_MAX);
			uint32_t index = _indexdata.at(id);
			uint32_t size;
			if (id + 1 == _indexdata.size())
				size = static_cast<uint32_t>(_data.size()) - index;
			else
				size = _indexdata.at(id + 1) - index;
			return std::make_pair(_data.get(index), size);
		}

		std::string toString() const {
			return std::to_string(_data.size()) + ":" + PriLib::Convert::to_hex(_data.get(), _data.size());
		}

	private:
		PriLib::lightlist<uint32_t> _indexdata;
		PriLib::lightlist<uint8_t> _data;
	};
}
