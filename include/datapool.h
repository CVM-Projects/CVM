#pragma once
#include <map>
#include <vector>
#include <cassert>
#include <memory>
#include <variant>
#include <mutex>
#include "../prilib/include/lightlist.h"
#include "../prilib/include/memory.h"
#include "../prilib/include/convert.h"
#include "../prilib/include/explicittype.h"
#include "config.h"
#include "inststruct/filenamemap.h"

namespace CVM
{
	class LiteralDataPoolCreater : public std::map<uint32_t, std::pair<uint8_t*, uint32_t>>
	{
	public:
		LiteralDataPoolCreater() = default;

		LiteralDataPoolCreater(const LiteralDataPoolCreater &) = delete;

		~LiteralDataPoolCreater() {
			for (auto &pair : *this) {
				delete pair.second.first;
			}
		}

	};

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

	DefineExplicitTypeWithValue(DataID, Config::DataIndexType, 0);
	inline bool operator<(const DataID &lhs, const DataID &rhs) {
		return lhs.data < rhs.data;
	}

	class FileLiteralDataPoolMap
	{
	public:
		using DataType = std::pair<MemorySize, const uint8_t*>;

	public:
		explicit FileLiteralDataPoolMap(const FileID &fileID) : _fileID(fileID) {}
		FileLiteralDataPoolMap(const FileLiteralDataPoolMap &) = delete;
		FileLiteralDataPoolMap& operator=(const FileLiteralDataPoolMap &) = delete;

		bool has(const DataID &dataid) {
			return this->_data.find(dataid) != this->_data.end();
		}
		bool insert(const DataID &dataid, const DataType &data) {
			if (!has(dataid)) {
				this->_data[dataid] = data;
				return true;
			}
			return false;
		}
		const DataType& operator[](const DataID &dataid) const {
			return this->_data.at(dataid);
		}
		const FileID& fileID() const {
			return _fileID;
		}

	private:
		FileID _fileID;
		std::map<DataID, DataType> _data;
	};

	class LiteralDataPoolMap
	{
	public:
		using DataType = FileLiteralDataPoolMap::DataType;

	public:
		LiteralDataPoolMap() = default;
		LiteralDataPoolMap(const LiteralDataPoolMap &) = delete;
		LiteralDataPoolMap& operator=(const LiteralDataPoolMap &) = delete;

		bool has(const FileID &fileid) {
			return this->_data.find(fileid) != this->_data.end();
		}
		bool insert(const FileID &fileid, const DataID &dataid, const DataType &data) {
			insert(fileid);
			return this->_data.at(fileid)->insert(dataid, data);
		}
		bool insert(const FileID &fileid) {
			if (!has(fileid)) {
				this->_data.insert({fileid, std::make_shared<FileLiteralDataPoolMap>(fileid)});
				return true;
			}
			return false;
		}
		FileLiteralDataPoolMap& operator[](const FileID &fileid) {
			return *this->_data.at(fileid);
		}
		const FileLiteralDataPoolMap& operator[](const FileID &fileid) const {
			return *this->_data.at(fileid);
		}

	private:
		std::map<FileID, std::shared_ptr<FileLiteralDataPoolMap>> _data;
	};

	class NewLiteralDataPool
	{
	public:
		explicit NewLiteralDataPool() {}
		NewLiteralDataPool(const NewLiteralDataPool &) = delete;
		NewLiteralDataPool& operator=(const NewLiteralDataPool &) = delete;

		~NewLiteralDataPool() {
			for (auto &data : _data_will_remove) {
				delete[] data.second;
			}
		}

		uint8_t* alloc(size_t size) {
			std::lock_guard<std::mutex> lock(_alloc_mutex);
			// TODO
			uint8_t *result = new uint8_t[size];
			_total_size += size;
			_data_will_remove.push_back({size, result});
			return result;
		}
		uint8_t* allocClear(size_t size) {
			uint8_t *data = alloc(size);
			std::memset(data, 0, size);
			return data;
		}
		std::string toString() const {
			// TODO
			std::string result = std::to_string(_total_size) + ":";
			for (auto &data : _data_will_remove) {
				result += PriLib::Convert::to_hex(data.second, data.first) + " ";
			}
			result.pop_back();
			return result;
			// return std::to_string(_data.size()) + ":" + PriLib::Convert::to_hex(_data.data(), _data.size());
		}
		LiteralDataPoolMap& dataPoolMap() {
			return _dataPoolMap;
		}
		const LiteralDataPoolMap& dataPoolMap() const {
			return _dataPoolMap;
		}

		// TODO: This function is temp.
		std::pair<const uint8_t*, uint32_t> at(Config::DataIndexType index) const {
		    const auto &result = _dataPoolMap[FileID(0)][DataID(index)];
		    return std::make_pair(result.second, result.first.data);
		}

	private:
		// std::vector<uint8_t> _data;
		size_t _total_size = 0;
		std::list<std::pair<size_t, uint8_t*>> _data_will_remove;
		LiteralDataPoolMap _dataPoolMap;
		mutable std::mutex _alloc_mutex;
	};
}
