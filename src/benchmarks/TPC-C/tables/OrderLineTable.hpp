/*
 * OrderLineTable.hpp
 *
 *  Created on: Feb 17, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_ORDERLINETABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_ORDERLINETABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "TPCCUtil.hpp"
#include <ctime>
#include <iostream>

using namespace tpcc_settings;

namespace TPCC{
class OrderLine{
public:
	//	Primary Key: (OL_W_ID, OL_D_ID, OL_O_ID, OL_NUMBER)
	//	(OL_W_ID, OL_D_ID, OL_O_ID) Foreign Key, references (O_W_ID, O_D_ID, O_ID)
	//	(OL_SUPPLY_W_ID, OL_I_ID) Foreign Key, references (S_W_ID, S_I_ID)
	// Size: 64 Bytes

	uint32_t 	OL_O_ID;			// 10,000,000 unique IDs
	uint8_t		OL_D_ID; 			// 20 unique IDs
	uint16_t	OL_W_ID;			// 2*W unique IDs
	uint8_t		OL_NUMBER;			// 15 unique IDs
	uint32_t	OL_I_ID;			// 200,000 unique IDs
	uint32_t 	OL_SUPPLY_W_ID;		// 2*W unique IDs
	time_t		OL_DELIVERY_D;		// date and time, or null
	uint8_t		OL_QUANTITY;		// numeric(2)
	float		OL_AMOUNT;			// signed numeric(6, 2)
	char		OL_DIST_INFO[25];	// fixed text, size 24

	void initialize(uint8_t olNumber, uint32_t oID, uint8_t dID, uint16_t wID, bool newOrder, TPCC::RandomGenerator& random, time_t now){
		OL_O_ID = oID;
		OL_D_ID = dID;
		OL_W_ID = wID;
		OL_NUMBER = olNumber;
		OL_I_ID = (uint32_t) random.number(ORDERLINE_MIN_I_ID, ORDERLINE_MAX_I_ID);
		OL_SUPPLY_W_ID = wID;
		OL_QUANTITY = (uint8_t) ORDERLINE_INITIAL_QUANTITY;
		if (!newOrder) {
			OL_AMOUNT = 0.00;
			OL_DELIVERY_D = now;
		} else {
			OL_AMOUNT = random.fixedPoint(2, ORDERLINE_MIN_AMOUNT, ORDERLINE_MAX_AMOUNT);
			// HACK: Empty delivery date == null
			// OL_DELIVERY_D[0] = '\0';
		}
		random.astring(OL_DIST_INFO, (int)sizeof(OL_DIST_INFO) - 1, (int)sizeof(OL_DIST_INFO)-1);
	}

	friend std::ostream& operator<<(std::ostream& os, const OrderLine& ol) {
		os << "NO_O_ID:" << (int)ol.OL_O_ID << "|OL_D_ID:" << (int)ol.OL_D_ID << "|OL_W_ID:" << (int)ol.OL_W_ID << "|OL_NUMBER:" << (int)ol.OL_NUMBER << "|OL_I_ID:" << (int)ol.OL_I_ID;
		return os;
	}

};

class OrderLineVersion{
public:
	OrderLine orderLine;
	Timestamp writeTimestamp;
};

class OrderLineTable{
public:
	RDMARegion<OrderLineVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<OrderLineVersion>	*olderVersions;

	OrderLineTable(size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<OrderLineVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<OrderLineVersion>(size * maxVersionsCnt, baseContext, mrFlags);
	}

	void insert(uint8_t olNumber, uint32_t oID, uint8_t dID, uint16_t wID,  bool newOrder, TPCC::RandomGenerator& random, time_t now, Timestamp &ts) {
		size_t ind = ( (
				wID * config::tpcc_settings::DISTRICT_PER_WAREHOUSE
				+ dID) * config::tpcc_settings::ORDER_PER_DISTRICT
				+ oID) * ORDER_MAX_OL_CNT + olNumber;

		headVersions->getRegion()[ind].orderLine.initialize(olNumber, oID, dID, wID, newOrder, random, now);
		headVersions->getRegion()[ind].writeTimestamp.copy(ts);
	}

	void getMemoryHandler(MemoryHandler<OrderLineVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<OrderLineVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	~OrderLineTable(){
		DEBUG_COUT("OrderLineTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	size_t size_;
	size_t maxVersionsCnt_;
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_ORDERLINETABLE_HPP_ */