/*
 * StockTable.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_STOCKTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_STOCKTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "../../../../config.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "TPCCUtil.hpp"
#include <iostream>
#include <set>	// for std::set

using namespace tpcc_settings;

namespace TPCC{

class Stock{
public:
	//	Primary Key: (S_W_ID, S_I_ID)
	//	S_W_ID Foreign Key, references W_ID
	//	S_I_ID Foreign Key, references I_ID
	//	Size: 320 B

	uint32_t	S_I_ID;			// 200,000 unique IDs 100,000 populated per warehouse
	uint16_t	S_W_ID;			// 2*W unique IDs
	uint16_t	S_QUANTITY;		// signed numeric(4)
	char		S_DIST[config::tpcc_settings::DISTRICT_PER_WAREHOUSE][25];	// each fixed text, size 24
	uint32_t	S_YTD;			// numeric(8)
	uint16_t	S_ORDER_CNT;	// numeric(4)
	uint16_t	S_REMOTE_CNT;	// numeric(4)
	char		S_DATA[51];		// variable text, size 50 Make information

	void initialize(uint32_t iID, uint16_t wID, bool original, TPCC::RandomGenerator& random) {
		S_I_ID = iID;
		S_W_ID = wID;
		S_QUANTITY = (uint16_t)random.number(STOCK_MIN_QUANTITY, STOCK_MAX_QUANTITY);
		S_YTD = 0;
		S_ORDER_CNT = 0;
		S_REMOTE_CNT = 0;
		for (size_t i = 0; i < config::tpcc_settings::DISTRICT_PER_WAREHOUSE; ++i) {
			random.astring(S_DIST[i], sizeof(S_DIST[i]), sizeof(S_DIST[i]));
		}
		random.astring(S_DATA, STOCK_MIN_DATA, STOCK_MAX_DATA);

		if (original) {
			setOriginal(random, S_DATA);
		}
	}

	static void setOriginal(TPCC::RandomGenerator& random, char* s) {
		int length = static_cast<int>(strlen(s));
		int position = random.number(0, length-8);
		std::memcpy(s + position, "ORIGINAL", 8);
	}

	friend std::ostream& operator<<(std::ostream& os, const Stock& s) {
		os << "S_I_ID:" << (int)s.S_I_ID << "|S_W_ID:" << (int)s.S_W_ID;
		return os;
	}
};

class StockVersion{
public:
	Timestamp writeTimestamp;
	Stock stock;

	static size_t getOffsetOfStock(){
		return offsetof(StockVersion, stock);
	}

	static size_t getOffsetOfTimestamp(){
		return offsetof(StockVersion, writeTimestamp);
	}

	friend std::ostream& operator<<(std::ostream& os, const StockVersion& s) {
		os << s.stock << " (" << s.writeTimestamp << ")";
		return os;
	}
};

class StockTable{
public:
	RDMARegion<StockVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<StockVersion>	*olderVersions;

	StockTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<StockVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<StockVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "StockTable", __func__, "[Info] Stock table initialized");
	}

	void populate(size_t warehouseOffset, uint16_t wID, TPCC::RandomGenerator& random, size_t itemsCnt, Timestamp& ts){
		// Select 10% of the stock to be marked "original"
		//std::set<int> selected_rows = random.selectUniqueIds(itemsCnt/10, 0, (int)(itemsCnt - 1));

		for (uint32_t i = 0; i < itemsCnt; ++i) {
			bool is_original = (random.number(1, 10) == 10);
			//bool is_original = selected_rows.find(i) != selected_rows.end();
			headVersions->getRegion()[warehouseOffset * itemsCnt + i].stock.initialize(i, wID, is_original, random);
			headVersions->getRegion()[warehouseOffset * itemsCnt + i].writeTimestamp.copy(ts);
		}
	}

	void getMemoryHandler(MemoryHandler<StockVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<StockVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	~StockTable(){
		DEBUG_WRITE(os_, "StockTable", __func__, "[Info] Deconstructor called");
		delete headVersions;
		delete tsList;
		delete olderVersions;
	}

private:
	std::ostream &os_;
	size_t size_;
	size_t maxVersionsCnt_;
};

} // namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_TABLES_STOCKTABLE_HPP_ */
