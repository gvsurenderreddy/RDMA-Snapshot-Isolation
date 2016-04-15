/*
 * OrderTable.hpp
 *
 *  Created on: Feb 17, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_ORDERTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_ORDERTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "TPCCUtil.hpp"
#include "../../../index/hash/HashIndex.hpp"
#include <ctime>
#include <string>
#include <iostream>


using namespace tpcc_settings;

namespace TPCC{

class Order{
public:
	// Primary Key: (O_W_ID, O_D_ID, O_ID)
	// (O_W_ID, O_D_ID, O_C_ID) Foreign Key, references (C_W_ID, C_D_ID, C_ID)
	// Size: 32 Bytes

	uint32_t	O_ID;			// 10,000,000 unique IDs
	uint8_t 	O_D_ID;			// 20 unique IDs
	uint16_t 	O_W_ID;			// 2*W unique IDs
	uint32_t 	O_C_ID;			// 96,000 unique IDs
	time_t		O_ENTRY_D;		// date and time
	uint8_t		O_CARRIER_ID;	// 10 unique IDs, or null
	uint8_t		O_OL_CNT;		// numeric(2) Count of Order-Lines
	uint8_t		O_ALL_LOCAL;	// numeric(1)

	void initialize(uint32_t oID, uint32_t cID, uint8_t dID, uint16_t wID, bool newOrder,  TPCC::RandomGenerator& random, time_t now) {
		O_ID = oID;
		O_C_ID = cID;
		O_D_ID = dID;
		O_W_ID = wID;
		if (!newOrder) {
			O_CARRIER_ID = (unsigned char)random.number(ORDER_MIN_CARRIER_ID, ORDER_MAX_CARRIER_ID);
		} else {
			O_CARRIER_ID = (unsigned char)ORDER_NULL_CARRIER_ID;
		}
		O_OL_CNT = (unsigned char)random.number(ORDER_MIN_OL_CNT, ORDER_MAX_OL_CNT);
		O_ALL_LOCAL = (unsigned char)ORDER_INITIAL_ALL_LOCAL;
		O_ENTRY_D = now;
	}

	friend std::ostream& operator<<(std::ostream& os, const Order& o) {
		os << "O_ID:" << (int)o.O_ID << "|O_D_ID:" << (int)o.O_D_ID << "|O_W_ID:"
				<< (int)o.O_W_ID << "|O_C_ID:" << (int)o.O_C_ID << "|O_OL_CNT:" << (int)o.O_OL_CNT;
		return os;
	}
};

class OrderVersion{
public:
	Timestamp writeTimestamp;
	Order order;

	static size_t getOffsetOfTimestamp(){
		return offsetof(OrderVersion, writeTimestamp);
	}
};

class OrderTable{
private:
	std::ostream &os_;
	struct OrderAddressIdentifier {
		primitive::client_id_t clientWhoOrdered;
		size_t clientRegionOffset;
	};

	HashIndex<std::string, uint32_t> largestOrderForCustomer_Index_;	// the key is concatenation of wID, dID, and cID
	HashIndex<std::string, OrderAddressIdentifier> orderToMemoryAddress_Index_;	// the key is concatentation of wID, dID, and oID

public:
	RDMARegion<OrderVersion> 	*headVersions;
	RDMARegion<Timestamp> 		*tsList;
	RDMARegion<OrderVersion>	*olderVersions;

	OrderTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<OrderVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<OrderVersion>(size * maxVersionsCnt, baseContext, mrFlags);
	}

	//	void insert(uint32_t oID, uint32_t cID, uint8_t dID, uint16_t wID, bool newOrder,  TPCC::RandomGenerator& random, time_t now, Timestamp &ts){
	//		size_t ind =
	//				(wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE
	//						+ dID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT
	//						+ oID;
	//		headVersions->getRegion()[ind].order.initialize(oID, cID, dID, wID, newOrder, random, now);
	//		headVersions->getRegion()[ind].writeTimestamp.copy(ts);
	//	}
	//
	//	void insert(size_t warehouseOffset, Order &order, Timestamp &ts) {
	//		size_t ind =
	//				(warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE
	//						+ order.O_D_ID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT
	//						+ order.O_ID;
	//		std::memcpy(&headVersions->getRegion()[ind].order, &order, sizeof(Order));
	//		headVersions->getRegion()[ind].writeTimestamp.copy(ts);
	//	}

	void getMemoryHandler(MemoryHandler<OrderVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<OrderVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	void registerOrderInIndex(uint16_t warehouseOffset, uint8_t dID, uint32_t cID, uint32_t oID, primitive::client_id_t clientWhoOrdered, size_t regionOffset) {
		// First, register its physical address
		OrderAddressIdentifier addr;
		addr.clientWhoOrdered = clientWhoOrdered;
		addr.clientRegionOffset = regionOffset;
		std::string key1 = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		orderToMemoryAddress_Index_.put(key1, addr);

		// then, update the customer's biggest order, if necessary
		std::string key2 = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID) + "_c_" + std::to_string(cID);
		if (largestOrderForCustomer_Index_.hasKey(key2)) {
			uint32_t existingOID = largestOrderForCustomer_Index_.get(key2);
			if (existingOID < oID)
				largestOrderForCustomer_Index_.put(key2, oID);
		}
		else largestOrderForCustomer_Index_.put(key2, oID);
	}

	uint32_t getBiggestOrderIDForCustomer(uint16_t warehouseOffset, uint8_t dID, uint32_t cID){
		std::string key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID) + "_c_" + std::to_string(cID);
		return largestOrderForCustomer_Index_.get(key);
	}

	void getOrderMemoryAddress(uint16_t warehouseOffset, uint8_t dID, uint32_t oID, primitive::client_id_t *clientWhoOrdered_OUTPUT, size_t *regionOffset_OUTPUT){
		std::string key = "w_" + std::to_string(warehouseOffset) + "_d_" + std::to_string(dID) + "_o_" + std::to_string(oID);
		OrderAddressIdentifier addr = orderToMemoryAddress_Index_.get(key);
		*clientWhoOrdered_OUTPUT = addr.clientWhoOrdered;
		*regionOffset_OUTPUT = addr.clientRegionOffset;
	}

	void clearIndex(){
		orderToMemoryAddress_Index_.clear();
		largestOrderForCustomer_Index_.clear();
	}

	bool isIndexEmpty(){
		return (orderToMemoryAddress_Index_.size() == 0 && largestOrderForCustomer_Index_.size() == 0);
	}

	~OrderTable(){
		DEBUG_WRITE(os_, "OrderTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	size_t size_;
	size_t maxVersionsCnt_;
};
}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_ORDERTABLE_HPP_ */
