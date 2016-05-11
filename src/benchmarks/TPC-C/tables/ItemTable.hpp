/*
 * ItemTable.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_ITEMTABLE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_ITEMTABLE_HPP_

#include "../../../basic-types/timestamp.hpp"
#include "TPCCUtil.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../rdma-region/MemoryHandler.hpp"
#include <iostream>

using namespace tpcc_settings;



namespace TPCC {

class Item{
public:
	// Primary Key: I_ID
	// Size: 88 Bytes
	uint32_t 	I_ID;		// 200,000 unique IDs 100,000 items are populated
	uint32_t 	I_IM_ID;	// 200,000 unique IDs Image ID associated to Item
	char		I_NAME[24];	// variable text, size 24
	float		I_PRICE;	// numeric(5, 2)
	char		I_DATA[50];	// variable text, size 50 Brand information

	void initialize(uint32_t id, bool original, TPCC::RandomGenerator& random){
		I_ID = id;
		I_IM_ID = (uint32_t)random.number(ITEM_MIN_IM, ITEM_MAX_IM);
		I_PRICE = random.fixedPoint(2, ITEM_MIN_PRICE, ITEM_MAX_PRICE);
		random.astring(I_NAME, ITEM_MIN_NAME, ITEM_MAX_NAME);
		random.astring(I_DATA, ITEM_MIN_DATA, ITEM_MAX_DATA);

		if (original) {
			int length = static_cast<int>(strlen(I_DATA));
			int position = random.number(0, length - 8);
			std::memcpy(I_DATA + position, "ORIGINAL", 8);
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const Item& i) {
		os << "I_ID:" << (int)i.I_ID;
		return os;
	}
};


class ItemVersion{
public:
	Timestamp writeTimestamp;
	Item item;

	static size_t getOffsetOfItem(){
		return offsetof(ItemVersion, item);
	}

	friend std::ostream& operator<<(std::ostream& os, const ItemVersion& i) {
		os << i.item << " (" << i.writeTimestamp << ")";
		return os;
	}
};


class ItemTable{
public:
	RDMARegion<ItemVersion> *headVersions;
	RDMARegion<Timestamp> 	*tsList;
	RDMARegion<ItemVersion>	*olderVersions;

	ItemTable(std::ostream &os, size_t size, size_t maxVersionsCnt, RDMAContext &baseContext, int mrFlags)
	: os_(os),
	  size_(size),
	  maxVersionsCnt_(maxVersionsCnt){
		headVersions 	= new RDMARegion<ItemVersion>(size, baseContext, mrFlags);
		tsList 			= new RDMARegion<Timestamp>(size * maxVersionsCnt, baseContext, mrFlags);
		olderVersions	= new RDMARegion<ItemVersion>(size * maxVersionsCnt, baseContext, mrFlags);

		DEBUG_WRITE(os_, "ItemTable", __func__, "[Info] Item table initialized");
	}

	size_t getMaxVersionsCnt() const {
		return maxVersionsCnt_;
	}

	size_t getSize() const {
		return size_;
	}

	void getMemoryHandler(MemoryHandler<ItemVersion> &headVersionsMH, MemoryHandler<Timestamp> &tsListMH, MemoryHandler<ItemVersion> &olderVersionsMH){
		headVersions->getMemoryHandler(headVersionsMH);
		tsList->getMemoryHandler(tsListMH);
		olderVersions->getMemoryHandler(olderVersionsMH);
	}

	// Generates num_items items and inserts them into Item table.
	void populate(Timestamp &initialTimestamp, TPCC::RandomGenerator& random) {
		// Select 10% of the rows to be marked "original"
		std::set<int> original_rows = random.selectUniqueIds(size_/10, 1, (int)size_);

		for (unsigned int  i = 0; i < size_; ++i) {
			bool is_original = original_rows.find(i) != original_rows.end();
			headVersions->getRegion()[i].item.initialize(i, is_original, random);
			headVersions->getRegion()[i].writeTimestamp.copy(initialTimestamp);	// calls the assignment operator
		}
	}

	~ItemTable(){
		DEBUG_WRITE(os_, "ItemTable", __func__, "[Info] Deconstructor called");
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


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_ITEMTABLE_HPP_ */
