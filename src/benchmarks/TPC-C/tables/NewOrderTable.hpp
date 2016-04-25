/*
 * NewOrderTable.hpp
 *
 *  Created on: Feb 20, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_

#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../index/hash/SortedMultiValueHashIndex.hpp"
#include <string>
#include <utility>	// std::swap
#include <queue>	// std::priority_queue
#include <mutex>		// for std::lock_guard


using namespace tpcc_settings;


namespace TPCC{

class NewOrder{
public:
	// Primary Key: (NO_W_ID, NO_D_ID, NO_O_ID)
	// (NO_W_ID, NO_D_ID, NO_O_ID) Foreign Key, references (O_W_ID, O_D_ID, O_ID)
	// Size: 8 Bytes

	uint32_t 	NO_O_ID;	// 10,000,000 unique IDs
	uint8_t 	NO_D_ID;	// 20 unique IDs
	uint16_t	NO_W_ID;	// 2*W unique IDs

	void initialize(uint16_t wID, uint8_t dID, uint32_t oID){
		NO_D_ID = dID;
		NO_W_ID = wID;
		NO_O_ID = oID;
	}

	friend std::ostream& operator<<(std::ostream& os, const NewOrder& no) {
		os << "NO_O_ID:" << (int)no.NO_O_ID << "|NO_D_ID:" << (int)no.NO_D_ID << "|NO_W_ID:" << (int)no.NO_W_ID;
		return os;
	}
};

class NewOrderVersion{
public:
	Timestamp writeTimestamp;
	NewOrder newOrder;

	static size_t getOffsetOfTimestamp(){
		return offsetof(NewOrderVersion, writeTimestamp);
	}

	friend std::ostream& operator<<(std::ostream& os, const NewOrderVersion& v) {
		os << v.newOrder << "(" << v.writeTimestamp << ")";
		return os;
	}
};

class NewOrderTable{
public:
	RDMARegion<NewOrderVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<NewOrderVersion>	*olderVersions;

	NewOrderTable(std::ostream &os, size_t size, size_t warehouseCnt, size_t districtCnt, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt),
	  newOrderToMemoryAddress_Index_(warehouseCnt * districtCnt),
	  oldestNewOrderInDistrict_Index_(warehouseCnt * districtCnt){
		// oldestNewOrderInDistrict_Index_(warehouseCnt * districtCnt) {
		headVersions 	= new RDMARegion<NewOrderVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<NewOrderVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "NewOrderTable", __func__, "[Info] New Order table initialized");
	}

	//	void insert(size_t warehouseOffset, uint16_t wID, uint8_t dID, uint32_t oID, Timestamp &ts){
	//		size_t ind = ( warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE
	//						+ dID) * config::tpcc_settings::ORDER_PER_DISTRICT + oID;
	//
	//		headVersions->getRegion()[ind].newOrder.initialize(wID, dID, oID);
	//		headVersions->getRegion()[ind].writeTimestamp.copy(ts);
	//	}

	void getMemoryHandler(MemoryHandler<NewOrderVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<NewOrderVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	void registerNewOrderInIndex(uint16_t warehouseOffset, uint8_t dID, uint32_t oID, primitive::client_id_t clientWhoOrdered, size_t newOrderRegionOffset) {
		// First, register its physical address
		NewOrderAddressIdentifier addr(oID, clientWhoOrdered, newOrderRegionOffset);
		//std::string key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		size_t ind = (size_t) (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);
		newOrderToMemoryAddress_Index_[ind].put(oID, addr);

		// Second, update the oldest new order per district
		//key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID);
		std::lock_guard<std::mutex> lock(writeLock);
		oldestNewOrderInDistrict_Index_[ind].push(addr);
	}

	void getOldestNewOrder(uint16_t warehouseOffset, uint8_t dID, uint32_t *oID_OUTPUT, primitive::client_id_t *clientWhoOrdered_OUTPUT, size_t *regionOffset_OUTPUT) {
		//std::string key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID);
		//const NewOrderAddressIdentifier &addr = oldestNewOrderInDistrict_Index_.top(key);
		size_t ind = (size_t) (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);

		std::lock_guard<std::mutex> lock(writeLock);
		if (oldestNewOrderInDistrict_Index_[ind].empty())
			throw std::out_of_range("Empty list");
		const NewOrderAddressIdentifier &addr = oldestNewOrderInDistrict_Index_[ind].top();
		*oID_OUTPUT = addr.getOID();
		*clientWhoOrdered_OUTPUT = addr.getClientWhoOrdered();
		*regionOffset_OUTPUT = addr.getClientRegionOffset();
	}

	void registerDelivery(uint16_t warehouseOffset, uint8_t dID, uint32_t oID){
		// TODO: is popping safe here?
		(void) oID;

		//std::string key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID);
		//oldestNewOrderInDistrict_Index_.pop(key);
		size_t ind = (size_t) (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);
		std::lock_guard<std::mutex> lock(writeLock);
		oldestNewOrderInDistrict_Index_[ind].pop();
	}

	void getNewOrderMemoryAddress(uint16_t warehouseOffset, uint8_t dID, uint32_t oID, primitive::client_id_t *clientWhoOrdered_OUTPUT, size_t *regionOffset_OUTPUT){
		//std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		// NewOrderAddressIdentifier addr = newOrderToMemoryAddress_Index_.get(key);

		size_t ind = (size_t) (warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);
		NewOrderAddressIdentifier addr = newOrderToMemoryAddress_Index_[ind].get(oID);
		*clientWhoOrdered_OUTPUT = addr.getClientWhoOrdered();
		*regionOffset_OUTPUT = addr.getClientRegionOffset();
	}

	~NewOrderTable(){
		DEBUG_WRITE(os_, "NewOrderTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	struct NewOrderAddressIdentifier {
	private:
		uint32_t oID_;
		primitive::client_id_t clientWhoOrdered_;
		size_t clientRegionOffset_;
	public:
		NewOrderAddressIdentifier(){}

		NewOrderAddressIdentifier(uint32_t oID, primitive::client_id_t clientWhoOrdered, size_t clientRegionOffset){
			oID_ = oID;
			clientWhoOrdered_ = clientWhoOrdered;
			clientRegionOffset_ = clientRegionOffset;
		}

		~NewOrderAddressIdentifier(){}

		uint32_t getOID() const{
			return oID_;
		}

		primitive::client_id_t getClientWhoOrdered() const {
			return clientWhoOrdered_;
		}

		size_t getClientRegionOffset() const {
			return clientRegionOffset_;
		}

		void swap(NewOrderAddressIdentifier & other) // the swap member function (should never fail!)
		{
			// swap all the members (and base subobject, if applicable) with other
			std::swap(oID_, other.oID_);
			std::swap(clientWhoOrdered_, other.clientWhoOrdered_);
			std::swap(clientRegionOffset_, other.clientRegionOffset_);
		}

		NewOrderAddressIdentifier& operator=(NewOrderAddressIdentifier other){
			swap(other);	// swap this with other
			return *this;	// by convention, always return *this
		}

		NewOrderAddressIdentifier(const NewOrderAddressIdentifier& other)
		: oID_(other.oID_),
		  clientWhoOrdered_(other.clientWhoOrdered_),
		  clientRegionOffset_(other.clientRegionOffset_){
		}

		bool operator>(const NewOrderAddressIdentifier &other) const{
			return oID_ > other.oID_;
		}
	};

	std::ostream &os_;
	size_t size_;
	size_t maxVersionsCnt_;
	std::vector<HashIndex<uint32_t, NewOrderAddressIdentifier> > newOrderToMemoryAddress_Index_;
	//std::vector<SortedMultiValueHashIndex<std::string, NewOrderAddressIdentifier> > oldestNewOrderInDistrict_Index_;
	std::vector<std::priority_queue<NewOrderAddressIdentifier, std::vector<NewOrderAddressIdentifier>, std::greater<NewOrderAddressIdentifier> > > oldestNewOrderInDistrict_Index_;
	std::mutex writeLock;

};

}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_ */
