/*
 * WarehouseTable.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_WAREHOUSETABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_WAREHOUSETABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "TPCCUtil.hpp"
#include <iostream>
#include <cstddef>	// for offsetof()


using namespace tpcc_settings;

namespace TPCC{

class Warehouse {
public:
	// Primary Key: W_ID
	// Size: 96 Bytes

	uint16_t	W_ID;			// 2*W unique IDs
	char 		W_NAME[11]; 	// variable text, size 10
	char 		W_STREET_1[20];	// variable text, size 20
	char 		W_STREET_2[20];	// variable text, size 20
	char 		W_CITY[20]; 	// variable text, size 20
	char 		W_STATE[3];		// fixed text, size 2
	char 		W_ZIP[9];		// fixed text, size 9
	float		W_TAX;			// signed numeric(4,4) Sales tax
	float		W_YTD;			// signed numeric(12,2) Year to date balance

	void initialize(uint16_t wID, TPCC::RandomGenerator& random) {
		W_ID = wID;
		W_TAX = makeTax(random);
		W_YTD = WAREHOUSE_INITIAL_YTD;
		random.astring(W_NAME, WAREHOUSE_MIN_NAME, WAREHOUSE_MAX_NAME);
		random.astring(W_STREET_1, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(W_STREET_2, ADDRESS_MIN_STREET, ADDRESS_MAX_STREET);
		random.astring(W_CITY, ADDRESS_MIN_CITY, ADDRESS_MAX_CITY);
		random.astring(W_STATE, ADDRESS_STATE, ADDRESS_STATE);
		makeZip(random, W_ZIP);
	}

	static size_t getOffsetOfTax(){
		return offsetof(Warehouse, W_TAX);
	}

	friend std::ostream& operator<<(std::ostream& os, const Warehouse& w) {
		os << "W_ID:" << (int)w.W_ID;
		return os;
	}

	static uint16_t getWarehouseOffsetOnServer(uint16_t wID){
		size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
		return (uint16_t)(wID - (serverNum * config::tpcc_settings::WAREHOUSE_PER_SERVER));
	}

	static size_t getServerNum(uint16_t wID){
		return (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	}
};


class WarehouseVersion{
public:
	Timestamp writeTimestamp;
	Warehouse warehouse;

	static size_t getOffsetOfWarehouse(){
		return offsetof(WarehouseVersion, warehouse);
	}

	static size_t getOffsetOfTimestamp(){
		return offsetof(WarehouseVersion, writeTimestamp);
	}

	friend std::ostream& operator<<(std::ostream& os, const WarehouseVersion& v) {
		os << v.warehouse << "(" << v.writeTimestamp << ")";
		return os;
	}
};

class WarehouseTable{
public:
	RDMARegion<WarehouseVersion>	*headVersions;
	RDMARegion<Timestamp> 			*tsList;
	RDMARegion<WarehouseVersion>	*olderVersions;

	WarehouseTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<WarehouseVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<WarehouseVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "WarehouseTable", __func__, "[Info] Warehouse table initialized with " << size << " tuples");
	}

	void insert(size_t warehouseOffset, uint16_t wID, TPCC::RandomGenerator& random, Timestamp& ts){
		headVersions->getRegion()[warehouseOffset].warehouse.initialize(wID, random);
		headVersions->getRegion()[warehouseOffset].writeTimestamp.copy(ts);
	}

	void getMemoryHandler(MemoryHandler<WarehouseVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<WarehouseVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	static size_t getRecordPositionInTable(size_t warehouseOffset){
		return warehouseOffset;
	}

	~WarehouseTable(){
		DEBUG_WRITE(os_, "WarehouseTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	std::ostream &os_;
	size_t size_;
	size_t maxVersionsCnt_;
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_WAREHOUSETABLE_HPP_ */
