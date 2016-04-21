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
	// Size: 100 Bytes

	uint16_t	W_ID;			// 2*W unique IDs
	char 		W_NAME[11]; 	// variable text, size 10
	char 		W_STREET_1[21];	// variable text, size 20
	char 		W_STREET_2[21];	// variable text, size 20
	char 		W_CITY[21]; 	// variable text, size 20
	char 		W_STATE[3];		// fixed text, size 2
	char 		W_ZIP[10];		// fixed text, size 9
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
private:
	std::ostream &os_;
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

		bool isLocked = false;
		bool isDeleted = true;
		primitive::client_id_t clientID = 0;
		primitive::timestamp_t timestamp = 0;
		primitive::version_offset_t versionOffset = 0;

		for (unsigned int  i = 0; i < size_; ++i) {
			for (size_t j = 0; j < maxVersionsCnt_; j++){
				tsList->getRegion()[i * maxVersionsCnt_ + j].setAll(isDeleted, isLocked, versionOffset, clientID, timestamp);
			}
		}
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

	~WarehouseTable(){
		DEBUG_WRITE(os_, "WarehouseTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	size_t size_;
	size_t maxVersionsCnt_;
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_WAREHOUSETABLE_HPP_ */
