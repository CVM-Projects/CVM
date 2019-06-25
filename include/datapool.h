#pragma once
#include <map>
#include <vector>
#include <cassert>
#include <memory>
#include "../prilib/include/lightlist.h"
#include "../prilib/include/convert.h"
#include "../prilib/include/explicittype.h"
#include "config.h"
#include "inststruct/filenamemap.h"

namespace CVM
{
	DefineExplicitTypeWithValue(DataID, Config::DataIndexType, 0);
	inline bool operator<(const DataID &lhs, const DataID &rhs) {
		return lhs.data < rhs.data;
	}

	DefineExplicitTypeWithValue(FileDataID, Config::FileDataIndexType, 0);
	inline bool operator<(const FileDataID &lhs, const FileDataID &rhs) {
		return lhs.data < rhs.data;
	}
	union FileDataIDConverter {
		struct {
			FileID::Type fileid;
			DataID::Type dataid;
		};
		FileDataID::Type data;
	};
	inline FileDataID operator,(const FileID &fileid, const DataID &dataid) {
		FileDataIDConverter converter;
		converter.fileid = fileid.data;
		converter.dataid = dataid.data;
		return FileDataID(converter.data);
	}
	inline std::pair<FileID, DataID> operator*(const FileDataID &filedataid) {
		FileDataIDConverter converter;
		converter.data = filedataid.data;
		return std::make_pair(FileID(converter.fileid), DataID(converter.dataid));
	}

	class LiteralDataPool
	{
		class FlatLiteralDataPoolMap {
		public:
			using DataType = std::pair<MemoryIndex, MemorySize>;

		public:
			FlatLiteralDataPoolMap() = default;
			FlatLiteralDataPoolMap(const FlatLiteralDataPoolMap &) = delete;
			FlatLiteralDataPoolMap& operator=(const FlatLiteralDataPoolMap &) = delete;

			bool has(const FileDataID &filedataid) const {
				return this->_data.find(filedataid) != this->_data.end();
			}
			bool insert(const FileDataID &filedataid, const DataType &data) {
				if (!has(filedataid)) {
					this->_data.insert({filedataid, data});
					return true;
				}
				return false;
			}
			const DataType& operator[](const FileDataID &filedataid) const {
				assert(has(filedataid));
				return this->_data.at(filedataid);
			}

		private:
			std::map<FileDataID, DataType> _data;
		};
	public:
		explicit LiteralDataPool() : _dataPoolMap(new FlatLiteralDataPoolMap()) {}
		LiteralDataPool(const LiteralDataPool &) = delete;
		LiteralDataPool& operator=(const LiteralDataPool &) = delete;

		// TODO: This function is temp.
		std::pair<const uint8_t*, MemorySize> at(const FileDataID & filedataid) const {
			const auto &result = (*_dataPoolMap)[filedataid];
			return std::make_pair(_data.get(result.first.data), result.second);
		}

		bool has(const FileDataID &filedataid) const {
			return _dataPoolMap->has(filedataid);
		}
		bool has(const FileID &fileid, const DataID &dataid) const {
			return has((fileid, dataid));
		}

		std::string toString() const {
			return std::to_string(_data.size()) + ":" + PriLib::Convert::to_hex(_data.get(), _data.size());
		}

	private:
		std::unique_ptr<FlatLiteralDataPoolMap> _dataPoolMap;
		PriLib::lightlist<uint8_t> _data;
		friend class LiteralDataPoolCreator;
	};
}
