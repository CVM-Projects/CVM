#pragma once
#include <vector>
#include <utility>
#include "funcinfo.h"
#include "config.h"
#include "instruction.h"

namespace CVM::InstStruct
{
	using InstList = std::vector<Instruction*>; // TODO
//	using ParsedFunctionInfo = std::vector<std::pair<HashID, Instruction*>>;

	class LabelKeyTable
	{
	public:
		LabelKeyTable() = default;
		LabelKeyTable(LabelKeyTable &&other)
				: _data(std::move(other._data)), _linedata(std::move(other._linedata)) {}

		size_t operator[](const HashID &key) {
			auto iter = _data.find(key);
			if (iter == _data.end()) {
				size_t id = _data.size();
				_data.insert({ key, id });
				_linedata.push_back(std::numeric_limits<Config::LineCountType>::max());
				return id;
			}
			else {
				return iter->second;
			}
		}
		void setLine(size_t id, Config::LineCountType value) {
			Config::LineCountType &line = _linedata[id];
			assert(!hasValue(id));
			line = value;
		}
		Config::LineCountType getLine(size_t id) {
			Config::LineCountType line = _linedata[id];
			assert(hasValue(id));
			return line;
		}
		bool hasValue(size_t id) {
			Config::LineCountType line = _linedata[id];
			return line != std::numeric_limits<Config::LineCountType>::max();
		}

		void clear() {
			_data.clear();
			_linedata.clear();
		}

	private:
		std::map<HashID, size_t> _data;
		std::vector<Config::LineCountType> _linedata;
	};

	struct Function
	{
		explicit Function() = default;
		explicit Function(const Function &info) = default;

		explicit Function(Function &&func)
			: info(std::move(func.info)), instdata(std::move(func.instdata)), labelkeytable(std::move(func.labelkeytable)) {}

		~Function() {
			// TODO
			for (auto &p : instdata)
				if (p)
					delete p;
		}

		FunctionInfo info;
		InstList instdata;
		LabelKeyTable labelkeytable;
	};
}
