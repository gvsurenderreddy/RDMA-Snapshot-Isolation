/*
 * NewOrderTable.hpp
 *
 *  Created on: Feb 20, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_

#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../index/hash/HashIndex.hpp"
#include <string>

using namespace tpcc_settings;


namespace TPCC{

class NewOrder{
public:
	// Primary Key: (NO_W_ID, NO_D_ID, NO_O_ID)
	// (NO_W_ID, NO_D_ID, NO_O_ID) Foreign Key, references (O_W_ID, O_D_ID, O_ID)
	// Size: 12 Bytes

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
};


class NewOrderTable{
private:
	std::ostream &os_;

	struct NewOrderAddressIdentifier {
		uint32_t oID;
		primitive::client_id_t clientWhoOrdered;
		size_t clientRegionOffset ;
	};

	HashIndex<std::string, NewOrderAddressIdentifier> newOrderToMemoryAddress_Index_;
	HashIndex<std::string, NewOrderAddressIdentifier> oldestNewOrderInDistrict_Index_;

public:
	RDMARegion<NewOrderVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<NewOrderVersion>	*olderVersions;

	NewOrderTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<NewOrderVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<NewOrderVersion>(size * maxVersionsCnt, baseContext, mrFlags);
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

	void registerNewOrderInIndex(uint16_t wID, uint8_t dID, uint32_t oID, primitive::client_id_t clientWhoOrdered, size_t newOrderRegionOffset) {
		// First, register its physical address
		NewOrderAddressIdentifier addr;
		addr.oID = oID;
		addr.clientWhoOrdered = clientWhoOrdered;
		addr.clientRegionOffset = newOrderRegionOffset;
		std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		newOrderToMemoryAddress_Index_.put(key, addr);

		// Second, update the oldest new order per district
		key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID);
		if (oldestNewOrderInDistrict_Index_.hasKey(key)){
			uint32_t existingOID = oldestNewOrderInDistrict_Index_.get(key).oID;
			if (oID < existingOID)
				oldestNewOrderInDistrict_Index_.put(key, addr);
		}
		else oldestNewOrderInDistrict_Index_.put(key, addr);
	}

	void getOldestNewOrder(uint16_t wID, uint8_t dID, uint32_t *oID_OUTPUT, primitive::client_id_t *clientWhoOrdered_OUTPUT, size_t *regionOffset_OUTPUT) {
		std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID);
		NewOrderAddressIdentifier addr = oldestNewOrderInDistrict_Index_.get(key);
		*oID_OUTPUT = addr.oID;
		*clientWhoOrdered_OUTPUT = addr.clientWhoOrdered;
		*regionOffset_OUTPUT = addr.clientRegionOffset;
	}

	void getNewOrderMemoryAddress(uint16_t wID, uint8_t dID, uint32_t oID, primitive::client_id_t *clientWhoOrdered_OUTPUT, size_t *regionOffset_OUTPUT){
		std::string key = "w_" + std::to_string(wID) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		NewOrderAddressIdentifier addr = newOrderToMemoryAddress_Index_.get(key);
		*clientWhoOrdered_OUTPUT = addr.clientWhoOrdered;
		*regionOffset_OUTPUT = addr.clientRegionOffset;
	}

	~NewOrderTable(){
		DEBUG_WRITE(os_, "NewOrderTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	size_t size_;
	size_t maxVersionsCnt_;
};

}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_NEWORDERTABLE_HPP_ */
