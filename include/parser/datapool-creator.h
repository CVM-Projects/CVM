#pragma once
#include "datapool.h"
#include <mutex>
#include <optional>
#include "../prilib/include/memory.h"
#include "../prilib/include/convert.h"

namespace CVM
{
	class LiteralDataPoolMap
	{
	public:
		using DataType = std::pair<MemorySize, const uint8_t*>;

	public:
		explicit LiteralDataPoolMap() {}
		LiteralDataPoolMap(const LiteralDataPoolMap &) = delete;
		LiteralDataPoolMap& operator=(const LiteralDataPoolMap &) = delete;

		bool has(const DataID &dataid) const {
			assert(dataid.data != 0);
			return !(maxDataID() < dataid) && this->_data.at(dataid.data - 1);
		}
		void insert(const DataID &dataid, const DataType &data) {
			assert(!has(dataid));
			while (this->_data.size() < dataid.data) {
				this->_data.push_back(std::nullopt);
			}
			this->_data[dataid.data - 1] = data;
		}
		const DataType& operator[](const DataID &dataid) const {
			assert(dataid.data != 0);
			assert(has(dataid));
			return this->_data.at(dataid.data - 1).value();
		}
		DataID maxDataID() const {
			return DataID(static_cast<DataID::Type>(_data.size()));
		}
		auto begin() const {
			return _data.begin();
		}
		auto end() const {
			return _data.end();
		}

	private:
		std::vector<std::optional<DataType>> _data;
	};

	class LiteralDataPoolCreator
	{
	public:
		using DataType = LiteralDataPoolMap::DataType;

	public:
		explicit LiteralDataPoolCreator() : _dataPoolMap(new LiteralDataPoolMap()) {}
		LiteralDataPoolCreator(const LiteralDataPoolCreator &) = delete;
		LiteralDataPoolCreator& operator=(const LiteralDataPoolCreator &) = delete;

		~LiteralDataPoolCreator() {
			for (auto &data : _data_will_remove) {
				delete[] data.second;
			}
		}

		uint8_t* alloc(MemorySize size) {
			std::lock_guard<std::mutex> lock(_alloc_mutex);
			uint8_t *result = new uint8_t[size.data]();
			_total_size += size;
			_data_will_remove.push_back({size, result});
			return result;
		}
		std::string toString() const {
			std::string result = std::to_string(_total_size.data) + ":";
			for (auto &data : _data_will_remove) {
				result += PriLib::Convert::to_hex(data.second, data.first.data) + " ";
			}
			result.pop_back();
			return result;
		}
		bool has(const DataID& dataid) const {
			return _dataPoolMap->has(dataid);
		}
		void insert(const DataID& dataid, const DataType &data) {
			_dataPoolMap->insert(dataid, data);
		}

		void mergeTo(const FileID &fileid, LiteralDataPool &result);

	private:
		MemorySize _total_size;
		std::list<std::pair<MemorySize, uint8_t*>> _data_will_remove;
		std::unique_ptr<LiteralDataPoolMap> _dataPoolMap;
		mutable std::mutex _alloc_mutex;
	};

	inline void LiteralDataPoolCreator::mergeTo(const FileID& fileid, LiteralDataPool &result) {
		std::lock_guard<std::mutex> lock(_alloc_mutex);
		PriLib::lightlist<uint8_t> &_data = result._data;
		auto &_dataPoolMapReal = result._dataPoolMap;
		// TODO
		MemoryIndex index;
		_data.recapacity(_total_size.data);
		DataID dataid;
		for (const auto &dataid_value : *_dataPoolMap) {
			dataid = DataID(dataid.data + 1);
			if (dataid_value) {
				const DataType &data = dataid_value.value();
				uint8_t *address = _data.get(index.data);
				PriLib::Memory::copyTo(address, data.second, data.first.data);
				_dataPoolMapReal->insert((fileid, dataid), std::make_pair(index, data.first));
				index += data.first;
			}
		}
		_dataPoolMap.release();
		for (auto &data : _data_will_remove) {
			delete[] data.second;
		}
		_data_will_remove.clear();
	}
}
