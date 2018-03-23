#pragma once
#include "../prilib/include/lightlist.h"
#include "config.h"
#include "typeinfo.h"

namespace CVM
{
	class FunctionInfoAccesser
	{
	public:
		using StvarbTypeType = TypeIndex;
		using ArgumentTypeType = Config::RegisterIndexType;

		FunctionInfoAccesser(uint8_t *address)
			: _address(address) {}

		Config::RegisterIndexType& dyvarb_count() {
			return *reinterpret_cast<Config::RegisterIndexType*>(_address_dyvarb_count());
		}
		Config::RegisterIndexType& stvarb_count() {
			return *reinterpret_cast<Config::RegisterIndexType*>(_address_stvarb_count());
		}
		Config::RegisterIndexType& argument_count() {
			return *reinterpret_cast<Config::RegisterIndexType*>(_address_argument_count());
		}
		StvarbTypeType* sttypelist() {
			return reinterpret_cast<StvarbTypeType*>(_address_sttypelist());
		}
		ArgumentTypeType* arglist() {
			return reinterpret_cast<ArgumentTypeType*>(_address_arglist());
		}

		const Config::RegisterIndexType& dyvarb_count() const {
			return const_cast<const Config::RegisterIndexType&>(const_cast<FunctionInfoAccesser*>(this)->dyvarb_count());
		}
		const Config::RegisterIndexType& stvarb_count() const {
			return const_cast<const Config::RegisterIndexType&>(const_cast<FunctionInfoAccesser*>(this)->stvarb_count());
		}
		const Config::RegisterIndexType& argument_count() const {
			return const_cast<const Config::RegisterIndexType&>(const_cast<FunctionInfoAccesser*>(this)->argument_count());
		}
		const StvarbTypeType* sttypelist() const {
			return const_cast<const StvarbTypeType*>(const_cast<FunctionInfoAccesser*>(this)->sttypelist());
		}
		const ArgumentTypeType* arglist() const {
			return const_cast<const ArgumentTypeType*>(const_cast<FunctionInfoAccesser*>(this)->arglist());
		}

		static size_t GetSize(Config::RegisterIndexType stcount, Config::RegisterIndexType argcount) {
			return sizeof(Config::RegisterIndexType) * 3 + sizeof(StvarbTypeType) * stcount + sizeof(ArgumentTypeType) * argcount;
		}

	private:
		uint8_t *_address;

		uint8_t* _address_begin() {
			return _address;
		}
		uint8_t* _address_dyvarb_count() {
			return _address_begin();
		}
		uint8_t* _address_stvarb_count() {
			return _address_dyvarb_count() + sizeof(Config::RegisterIndexType);
		}
		uint8_t* _address_argument_count() {
			return _address_stvarb_count() + sizeof(Config::RegisterIndexType);
		}
		uint8_t* _address_sttypelist() {
			return _address_argument_count() + sizeof(Config::RegisterIndexType);
		}
		uint8_t* _address_arglist() {
			return _address_sttypelist() + sizeof(StvarbTypeType) * stvarb_count();
		}
		uint8_t* _address_end() {
			return _address_arglist() + sizeof(ArgumentTypeType) * argument_count();
		}
	};

	struct FunctionInfo
	{
	public:
		using StvarbTypeType = FunctionInfoAccesser::StvarbTypeType;
		using ArgumentTypeType = FunctionInfoAccesser::ArgumentTypeType;
		using Type = PriLib::lightlist<uint8_t>;

	public:
		FunctionInfo() = default;
		FunctionInfo(const FunctionInfo &info) = default;

		explicit FunctionInfo(FunctionInfo &&info) :
			data(std::move(info.data)) {}

		explicit FunctionInfo(size_t size)
			: data(size) {}

		FunctionInfo& operator=(const FunctionInfo &data) {
			this->data = data.data;
			return *this;
		}

	public:
		Type data;

	public:
		FunctionInfoAccesser get_accesser() {
			return FunctionInfoAccesser(const_cast<uint8_t*>(data.get()));
		}
		const FunctionInfoAccesser get_accesser() const {
			return FunctionInfoAccesser(const_cast<uint8_t*>(data.get()));
		}

	public:
		bool is_dyvarb(Config::RegisterIndexType index) const {
			return Config::is_dynamic(index, dyvarb_count(), stvarb_count());
		}
		bool is_stvarb(Config::RegisterIndexType index) const {
			return Config::is_static(index, dyvarb_count(), stvarb_count());
		}
		const StvarbTypeType& get_stvarb_type(Config::RegisterIndexType index) const {
			auto id = Config::get_static_id(index, dyvarb_count(), stvarb_count());
			return sttypelist()[id];
		}

		Config::RegisterIndexType dyvarb_count() const {
			return get_accesser().dyvarb_count();
		}
		Config::RegisterIndexType stvarb_count() const {
			return get_accesser().stvarb_count();
		}
		Config::RegisterIndexType argument_count() const {
			return get_accesser().argument_count();
		}
		Config::RegisterIndexType regsize() const {
			return dyvarb_count() + stvarb_count();
		}

		const StvarbTypeType* sttypelist() const {
			return get_accesser().sttypelist();
		}
		const ArgumentTypeType* arglist() const {
			return get_accesser().arglist();
		}
	};
}
