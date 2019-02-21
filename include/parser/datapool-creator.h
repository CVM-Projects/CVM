#pragma once
#include "datapool.h"
#include <mutex>
#include <optional>
#include "../prilib/include/memory.h"
#include "../prilib/include/convert.h"

namespace CVM
{
	class FileLiteralDataPoolMap
	{
	public:
		using DataType = std::pair<MemorySize, const uint8_t*>;

	public:
		explicit FileLiteralDataPoolMap(const FileID &fileID) : _fileID(fileID) {}
		FileLiteralDataPoolMap(const FileLiteralDataPoolMap &) = delete;
		FileLiteralDataPoolMap& operator=(const FileLiteralDataPoolMap &) = delete;

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
		const FileID& fileID() const {
			return _fileID;
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
		FileID _fileID;
		std::vector<std::optional<DataType>> _data;
	};

	class LiteralDataPoolMap
	{
	public:
		using DataType = FileLiteralDataPoolMap::DataType;

	public:
		LiteralDataPoolMap() = default;
		LiteralDataPoolMap(const LiteralDataPoolMap &) = delete;
		LiteralDataPoolMap& operator=(const LiteralDataPoolMap &) = delete;

		bool has(const FileID &fileid) const {
			return this->_data.find(fileid) != this->_data.end();
		}
		void insert(const FileID &fileid, const DataID &dataid, const DataType &data) {
			if (!has(fileid))
				insert(fileid);
			this->_data.at(fileid)->insert(dataid, data);
		}
		void insert(const FileID &fileid) {
			assert(!has(fileid));
			this->_data.insert({fileid, std::make_shared<FileLiteralDataPoolMap>(fileid)});
		}
		FileLiteralDataPoolMap& operator[](const FileID &fileid) {
			return *this->_data.at(fileid);
		}
		const FileLiteralDataPoolMap& operator[](const FileID &fileid) const {
			return *this->_data.at(fileid);
		}
		auto begin() const {
			return _data.begin();
		}
		auto end() const {
			return _data.end();
		}

	private:
		std::map<FileID, std::shared_ptr<FileLiteralDataPoolMap>> _data;
	};

	class LiteralDataPoolCreator
	{
	public:
		using DataType = FileLiteralDataPoolMap::DataType;

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
		bool has(const FileDataID &filedataid) const {
			FileID fileid;
			DataID dataid;
			std::tie(fileid, dataid) = *filedataid;
			return _dataPoolMap->has(fileid) && (*_dataPoolMap)[fileid].has(dataid);
		}
		bool has(const FileID &fileid, const DataID &dataid) const {
			return has((fileid, dataid));
		}
		void insert(const FileDataID &filedataid, const DataType &data) {
			FileID fileid;
			DataID dataid;
			std::tie(fileid, dataid) = *filedataid;
			_dataPoolMap->insert(fileid, dataid, data);
		}

		void merge(LiteralDataPool &result);

	private:
		MemorySize _total_size;
		std::list<std::pair<MemorySize, uint8_t*>> _data_will_remove;
		std::unique_ptr<LiteralDataPoolMap> _dataPoolMap;
		mutable std::mutex _alloc_mutex;
	};

	inline void LiteralDataPoolCreator::merge(LiteralDataPool &result) {
		std::lock_guard<std::mutex> lock(_alloc_mutex);
		PriLib::lightlist<uint8_t> &_data = result._data;
		auto &_dataPoolMapReal = result._dataPoolMap;
		// TODO
		MemorySize index;
		_data.recapacity(_total_size.data);
		for (const auto &fileid_value : *_dataPoolMap) {
			const FileID &fileid = fileid_value.first;
			DataID dataid;
			for (const auto &dataid_value : *fileid_value.second) {
				dataid = DataID(dataid.data + 1);
				if (dataid_value) {
					const DataType &data =  dataid_value.value();
					DataType newdata = data;
					newdata.second = _data.get(index.data);
					index += data.first;
					PriLib::Memory::copyTo(const_cast<uint8_t*>(newdata.second), data.second, data.first.data);
					_dataPoolMapReal->insert((fileid, dataid), newdata);
				}
			}
		}
		_dataPoolMap.release();
		for (auto &data : _data_will_remove) {
			delete[] data.second;
		}
		_data_will_remove.clear();
	}
}
