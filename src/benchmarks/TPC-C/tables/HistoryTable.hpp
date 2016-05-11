/*
 * HistoryTable.hpp
 *
 *  Created on: Feb 17, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_HISTORYTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_HISTORYTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "TPCCUtil.hpp"
#include <ctime>
#include <iostream>

using namespace tpcc_settings;

namespace TPCC{
class History{
public:
	//	Primary Key: none
	//	(H_C_W_ID, H_C_D_ID, H_C_ID) Foreign Key, references (C_W_ID, C_D_ID, C_ID)
	//	(H_W_ID, H_D_ID) Foreign Key, references (D_W_ID, D_ID)
	//  Size: 56 Bytes

	uint32_t	H_C_ID;		// 96,000 unique IDs
	uint8_t		H_C_D_ID;	// 20 unique IDs
	uint16_t 	H_C_W_ID; 	// 2*W unique IDs
	uint8_t 	H_D_ID;		// 20 unique IDs
	uint16_t 	H_W_ID;		// 2*W unique IDs
	time_t 		H_DATE; 	// date and time
	float 		H_AMOUNT; 	// signed numeric(6, 2)
	char 		H_DATA[24];	// variable text, size 24 Miscellaneous information

	void initialize(uint32_t cID, uint8_t dID, uint16_t wID, TPCC::RandomGenerator& random, time_t now) {
		H_C_ID = cID;
		H_C_D_ID = dID;
		H_D_ID = dID;
		H_C_W_ID = wID;
		H_W_ID = wID;
		H_AMOUNT = HISTORY_INITIAL_AMOUNT;
		H_DATE = now;
		random.astring(H_DATA, HISTORY_MIN_DATA, HISTORY_MAX_DATA);
	}

	friend std::ostream& operator<<(std::ostream& os, const History& h) {
		os << "H_C_ID:" << (int)h.H_C_ID << "|H_C_D_ID:" << (int)h.H_C_D_ID << "|H_C_W_ID:" << (int)h.H_C_W_ID << "|H_D_ID:" << (int)h.H_D_ID;
		return os;
	}
};

class HistoryVersion{
public:
	Timestamp writeTimestamp;
	History history;

	friend std::ostream& operator<<(std::ostream& os, const HistoryVersion& v) {
		os << v.history << "(" << v.writeTimestamp << ")";
		return os;
	}
};

class HistoryTable{
public:
	RDMARegion<HistoryVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<HistoryVersion>	*olderVersions;

	HistoryTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<HistoryVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<HistoryVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "HistoryTable", __func__, "[Info] History table initialized");
	}

	void getMemoryHandler(MemoryHandler<HistoryVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<HistoryVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	~HistoryTable(){
		DEBUG_WRITE(os_, "HistoryTable", __func__, "[Info] Deconstructor called");
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

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_HISTORYTABLE_HPP_ */
