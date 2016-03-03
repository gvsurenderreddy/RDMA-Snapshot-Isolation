/*
 * TPCCDB.cpp
 *
 *  Created on: Feb 16, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TPCCDB_CPP_
#define SRC_BENCHMARKS_TPC_C_TPCCDB_CPP_

#include "TPCCDB.hpp"
#include "../../../config.hpp"
#include "tables/TPCCUtil.hpp"
#include <set>

#define CLASS_NAME	"TPCCDB"


TPCC::TPCCDB::TPCCDB(size_t warehouseCnt, size_t districtCnt, size_t customerCnt, size_t orderCnt, size_t orderLineCnt, size_t newOrderCnt, size_t stockCnt, size_t itemCnt, size_t versionNum, TPCC::RealRandomGenerator& random, RDMAContext &context):
itemCnt_(itemCnt),
customerCnt_(customerCnt),
warehouseCnt_(warehouseCnt),
random_(random),
now_(std::time(nullptr)),
mrFlags_(IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC),
itemTable (itemCnt, versionNum, context, mrFlags_),
customerTable(customerCnt, versionNum, context, mrFlags_),
stockTable(stockCnt, versionNum, context, mrFlags_),
warehouseTable (warehouseCnt, versionNum, context, mrFlags_),
districtTable (districtCnt, versionNum, context, mrFlags_),
orderTable (orderCnt, versionNum, context, mrFlags_),
orderLineTable(orderLineCnt, versionNum, context, mrFlags_),
newOrderTable(newOrderCnt, versionNum, context, mrFlags_) {
	;
}

void TPCC::TPCCDB::populate() {
	Timestamp initialTS(0,0,0,0);

	// first, populate the item table
	itemTable.populate(initialTS, random_);

	for (unsigned short int wID = 0; wID < config::tpcc_settings::WAREHOUSE_PER_SERVER; wID++){
		// then populate the stock table for the given warehouse
		stockTable.populate(wID, random_, itemCnt_, initialTS);

		// insert the warehouse
		warehouseTable.insert(wID, random_, initialTS);

		for (unsigned char dID = 0; dID < config::tpcc_settings::DISTRICT_PER_WAREHOUSE; ++dID) {
			districtTable.insert(dID, wID, random_, initialTS);

			// Select 10% of the customers to have bad credit
			std::set<int> selected_rows =
					random_.selectUniqueIds(config::tpcc_settings::CUSTOMER_PER_DISTRICT/10, 1, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
			for (unsigned short int cID = 0; cID < config::tpcc_settings::CUSTOMER_PER_DISTRICT; ++cID) {
				bool bad_credit = selected_rows.find(cID) != selected_rows.end();
				customerTable.insert(cID, dID, wID, bad_credit, random_, now_, initialTS);
				//			History h;
				//			generateHistory(c_id, d_id, w_id, &h);
				//			tables->insertHistory(h);
			}

			// TODO: TPC-C 4.3.3.1. says that this should be a permutation of [1, 3000]. But since it is
			// for a c_id field, it seems to make sense to have it be a permutation of the customers.
			// For the "real" thing this will be equivalent
			int* permutation = random_.makePermutation(0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
			for (unsigned int oID = 0; oID < config::tpcc_settings::CUSTOMER_PER_DISTRICT; ++oID) {
				// The last new_orders_per_district_ orders are new
				bool isNewOrder = (unsigned int)(config::tpcc_settings::CUSTOMER_PER_DISTRICT - tpcc_settings::NEWORDER_INITIAL_NUM_PER_DISTRICT) < oID;
				TPCC::Order order;
				order.initialize(oID, (unsigned short int)permutation[oID], dID, wID, isNewOrder,  random_, now_);
				orderTable.insert(order, initialTS);

				// Generate each OrderLine for the order
				for (unsigned char olNumber = 0; olNumber < order.O_OL_CNT; ++olNumber) {
					orderLineTable.insert(olNumber, oID, dID, wID, isNewOrder, random_, now_, initialTS);
				}

				if (isNewOrder) {
					// This is a new order: make one for it
					newOrderTable.insert(wID, dID, oID, initialTS);
				}
			}
			delete[] permutation;
		}
	}
}

void TPCC::TPCCDB::getMemoryKeys(TPCC::ServerMemoryKeys *k){
	customerTable.getMemoryHandler(k->customerTableHeadVersions, k->customerTabletsList, k->customerTableOlderVersions);
	districtTable.getMemoryHandler(k->districtTableHeadVersions, k->districtTabletsList, k->districtTableOlderVersions);
	//historyTable.getMemoryHandler(k->historyTableHeadVersions, k->historyTabletsList, k->historyTableOlderVersions);
	itemTable.getMemoryHandler(k->itemTableHeadVersions, k->itemTabletsList, k->itemTableOlderVersions);
	newOrderTable.getMemoryHandler(k->newOrderTableHeadVersions, k->newOrderTabletsList, k->newOrderTableOlderVersions);
	orderLineTable.getMemoryHandler(k->orderLineTableHeadVersions, k->orderLineTabletsList, k->orderLineTableOlderVersions);
	orderTable.getMemoryHandler(k->orderTableHeadVersions, k->orderTabletsList, k->orderTableOlderVersions);
	stockTable.getMemoryHandler(k->stockTableHeadVersions, k->stockTabletsList, k->stockTableOlderVersions);
	warehouseTable.getMemoryHandler(k->warehouseTableHeadVersions, k->warehouseTabletsList, k->warehouseTableOlderVersions);

}

TPCC::TPCCDB::~TPCCDB(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called");
}

#endif /* SRC_BENCHMARKS_TPC_C_TPCCDB_CPP_ */