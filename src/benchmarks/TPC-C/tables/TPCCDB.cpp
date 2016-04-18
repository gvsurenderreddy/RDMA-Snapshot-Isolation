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
#include <cassert>


#define CLASS_NAME	"TPCCDB"


TPCC::TPCCDB::TPCCDB(std::ostream &os, std::vector<uint16_t> &warehouseIDs, size_t warehouseCnt, size_t districtCnt, size_t customerCnt, size_t orderCnt, size_t orderLineCnt, size_t newOrderCnt, size_t stockCnt, size_t itemCnt, size_t historyCnt, size_t versionNum, TPCC::RealRandomGenerator& random, RDMAContext &context):
os_(os),
itemCnt_(itemCnt),
customerCnt_(customerCnt),
warehouseCnt_(warehouseCnt),
random_(random),
now_(std::time(nullptr)),
mrFlags_(IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC),
warehouseIDs_(warehouseIDs),
itemTable (os_, itemCnt, versionNum, context, mrFlags_),
customerTable(os_, customerCnt, versionNum, context, mrFlags_),
stockTable(os_, stockCnt, versionNum, context, mrFlags_),
warehouseTable (os_, warehouseCnt, versionNum, context, mrFlags_),
districtTable (os_, districtCnt, versionNum, context, mrFlags_),
orderTable (os_, orderCnt, versionNum, context, mrFlags_),
orderLineTable(os_, orderLineCnt, versionNum, context, mrFlags_),
newOrderTable(os_, newOrderCnt, versionNum, context, mrFlags_),
historyTable(os_, historyCnt, versionNum, context, mrFlags_){

	populate();
	buildIndices();
}

void TPCC::TPCCDB::populate() {
	assert(warehouseIDs_.size() == config::tpcc_settings::WAREHOUSE_PER_SERVER);

	bool isLocked = false;
	bool isDeleted = false;
	primitive::client_id_t clientID = 0;
	primitive::timestamp_t timestamp = 0;
	primitive::version_offset_t versionOffset = 0;
	Timestamp initialTS(isDeleted, isLocked, versionOffset, clientID, timestamp);

	// first, populate the item table
	itemTable.populate(initialTS, random_);

	for (size_t warehouseOffset = 0; warehouseOffset < config::tpcc_settings::WAREHOUSE_PER_SERVER; warehouseOffset++){
		uint16_t wID = warehouseIDs_.at(warehouseOffset);
		// then populate the stock table for the given warehouse
		stockTable.populate(warehouseOffset, wID, random_, itemCnt_, initialTS);

		// insert the warehouse
		warehouseTable.insert(warehouseOffset, wID, random_, initialTS);

		for (unsigned char dID = 0; dID < config::tpcc_settings::DISTRICT_PER_WAREHOUSE; ++dID) {
			districtTable.insert(warehouseOffset, dID, wID, random_, initialTS);

			// Select 10% of the customers to have bad credit
			std::set<int> selected_rows =
					random_.selectUniqueIds(config::tpcc_settings::CUSTOMER_PER_DISTRICT/10, 1, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
			for (unsigned short int cID = 0; cID < config::tpcc_settings::CUSTOMER_PER_DISTRICT; ++cID) {
				bool bad_credit = selected_rows.find(cID) != selected_rows.end();
				customerTable.insert(warehouseOffset, cID, dID, wID, bad_credit, random_, now_, initialTS);
				//			History h;
				//			generateHistory(c_id, d_id, w_id, &h);
				//			tables->insertHistory(h);
			}

			// TODO: TPC-C 4.3.3.1. says that this should be a permutation of [1, 3000]. But since it is
			// for a c_id field, it seems to make sense to have it be a permutation of the customers.
			// For the "real" thing this will be equivalent
			//			int* permutation = random_.makePermutation(0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
			//			for (unsigned int oID = 0; oID < config::tpcc_settings::CUSTOMER_PER_DISTRICT; ++oID) {
			//				// The last new_orders_per_district_ orders are new
			//				bool isNewOrder = (unsigned int)(config::tpcc_settings::CUSTOMER_PER_DISTRICT - tpcc_settings::NEWORDER_INITIAL_NUM_PER_DISTRICT) < oID;
			//				TPCC::Order order;
			//				order.initialize(oID, (unsigned short int)permutation[oID], dID, wID, isNewOrder,  random_, now_);
			//				orderTable.insert(warehouseOffset, order, initialTS);
			//
			//				// Generate each OrderLine for the order
			//				for (unsigned char olNumber = 0; olNumber < order.O_OL_CNT; ++olNumber) {
			//					orderLineTable.insert(warehouseOffset, olNumber, oID, dID, wID, isNewOrder, random_, now_, initialTS);
			//				}
			//
			//				if (isNewOrder) {
			//					// This is a new order: make one for it
			//					newOrderTable.insert(warehouseOffset, wID, dID, oID, initialTS);
			//				}
			//			}
			//			delete[] permutation;
		}
	}
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] database populated successfully");
}

void TPCC::TPCCDB::buildIndices() {
	customerTable.buildIndexOnLastName();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Indices build successfully");
}

void TPCC::TPCCDB::getMemoryKeys(TPCC::ServerMemoryKeys *k){
	customerTable.getMemoryHandler(k->customerTableHeadVersions, k->customerTableTimestampList, k->customerTableOlderVersions);
	districtTable.getMemoryHandler(k->districtTableHeadVersions, k->districtTableTimestampList, k->districtTableOlderVersions);
	historyTable.getMemoryHandler(k->historyTableHeadVersions, k->historyTableTimestampList, k->historyTableOlderVersions);
	itemTable.getMemoryHandler(k->itemTableHeadVersions, k->itemTableTimestampList, k->itemTableOlderVersions);
	newOrderTable.getMemoryHandler(k->newOrderTableHeadVersions, k->newOrderTableTimestampList, k->newOrderTableOlderVersions);
	orderLineTable.getMemoryHandler(k->orderLineTableHeadVersions, k->orderLineTableTimestampList, k->orderLineTableOlderVersions);
	orderTable.getMemoryHandler(k->orderTableHeadVersions, k->orderTableTimestampList, k->orderTableOlderVersions);
	stockTable.getMemoryHandler(k->stockTableHeadVersions, k->stockTableTimestampList, k->stockTableOlderVersions);
	warehouseTable.getMemoryHandler(k->warehouseTableHeadVersions, k->warehouseTableTimestampList, k->warehouseTableOlderVersions);
}

void TPCC::TPCCDB::handleLargestOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::LargestOrderForCustomerIndexRespMsg &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::LARGEST_ORDER_FOR_CUSTOMER_INDEX;

	// first find the biggest orderID
	uint16_t warehouseOffset = req.parameters.largestOrderIndex.warehouseOffset;
	uint8_t dID = req.parameters.largestOrderIndex.dID;
	uint32_t cID = req.parameters.largestOrderIndex.cID;;
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: Largest_Order_For_Customer. Parameters: warehouseOffset = " << (int)warehouseOffset << ", dID = " << (int)dID << ", cID = " << cID);
	try{
		res.oID = orderTable.getBiggestOrderIDForCustomer(warehouseOffset, dID, cID);
		orderTable.getOrderMemoryAddress(warehouseOffset, dID, res.oID, &res.clientWhoOrdered, &res.orderRegionOffset);
		orderLineTable.getOrderLineMemoryAddress(warehouseOffset, dID, res.oID, &res.clientWhoOrdered, &res.orderLineRegionOffset, &res.numOfOrderlines);
		res.isSuccessful = true;
	}
	catch (const std::exception& e) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] The customer (wID: " << (int)warehouseOffset << ", dID: " << (int)dID << ", cID: " << (int)cID  <<  ") has not registered any order yet");
		res.isSuccessful = false;
	}
}

void TPCC::TPCCDB::handleCustomerNameIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::CustomerNameIndexRespMsg &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::CUSTOMER_LAST_NAME_INDEX;

	uint16_t wID = req.parameters.lastNameIndex.warehouseOffset;
	uint8_t dID = req.parameters.lastNameIndex.dID;
	std::string lastName = std::string(req.parameters.lastNameIndex.customerLastName);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: C_LastName_TO_C_ID. Parameters: wID = " << (int)wID << ", dID = " << (int)dID << ", lastName = " << lastName );

	assert(req.operationType == TPCC::IndexRequestMessage::OperationType::LOOKUP);
	try{
		res.cID = customerTable.getMiddleIDByLastName(wID, dID, lastName);
		res.isSuccessful = true;
	}
	catch (const std::exception& e) {
		PRINT_CERR(CLASS_NAME, __func__, "Looking for a non-existing customer");
		res.cID = -1;
		res.isSuccessful = false;
	}
}

void TPCC::TPCCDB::handleRegisterOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::IndexResponseMessage &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::REGISTER_ORDER;

	// first find the biggest orderID
	uint16_t warehouseOffset = req.parameters.registerOrderIndex.warehouseOffset;
	uint8_t dID = req.parameters.registerOrderIndex.dID;
	uint32_t cID = req.parameters.registerOrderIndex.cID;
	uint32_t oID = req.parameters.registerOrderIndex.oID;
	primitive::client_id_t clientWhoOrdered = req.clientID;
	size_t orderRegionOffset = req.parameters.registerOrderIndex.orderRegionOffset;
	size_t newOrderRegionOffset = req.parameters.registerOrderIndex.newOrderRegionOffset;
	size_t orderLineRegionOffset = req.parameters.registerOrderIndex.orderLineRegionOffset;
	uint8_t numOfOrderlines = req.parameters.registerOrderIndex.numOfOrderlines;

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: Register_Order. Params: warehouseOffset = " << (int)warehouseOffset << ", dID = " << (int)dID << ", cID = " << cID
			<< ", oID = " << oID << ", order offset: " << orderRegionOffset << ", neworder offset: " << newOrderRegionOffset
			<< ", orderline offset: " <<  orderLineRegionOffset << ", #orderlines: " << (int)numOfOrderlines);

	orderTable.registerOrderInIndex(warehouseOffset, dID, cID, oID, clientWhoOrdered, orderRegionOffset);
	newOrderTable.registerNewOrderInIndex(warehouseOffset, dID, oID, clientWhoOrdered, newOrderRegionOffset);
	orderLineTable.registerOrderLineInIndex(warehouseOffset, dID, oID, numOfOrderlines, clientWhoOrdered, orderLineRegionOffset);
	res.isSuccessful = true;
}

void TPCC::TPCCDB::handleLast20OrdersIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::Last20OrdersIndexResMsg &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::ITEMS_FOR_LAST_20_ORDERS;
	res.orderlinesCnt = 0;

	uint16_t warehouseOffset = req.parameters.last20OrdersIndex.warehouseOffset;
	uint8_t dID = req.parameters.last20OrdersIndex.dID;
	uint32_t dNextOID = req.parameters.last20OrdersIndex.D_NEXT_O_ID;
	primitive::client_id_t clientID;
	size_t regionOffset;
	uint8_t orderLineCnt;

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: Last_20_Orders. Parameters: warehouseOffset = " << (int)warehouseOffset << ", dID = " << (int)dID << ", D_NEXT_O_ID = " << dNextOID);

	if (dNextOID == 0) {
		res.isSuccessful = true;
		return;
	}

	int32_t oID_s = (int32_t)dNextOID - 20;
	uint32_t oID = (oID_s < 0 ) ? 0 : (uint32_t)oID_s;

	std::set<uint32_t> distinctIDs;
	try{

		for (; oID < dNextOID; oID++){
			orderLineTable.getOrderLineMemoryAddress(warehouseOffset, dID, oID, &clientID, &regionOffset, &orderLineCnt);

			for (uint8_t i = 0; i < orderLineCnt; i++){
				size_t ind = (unsigned)((clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + (regionOffset + i));
				distinctIDs.insert(orderLineTable.headVersions->getRegion()[ind].orderLine.OL_I_ID);
			}
		}
	}
	catch (const std::out_of_range& oor) {
		// possible cause: despite the fact that the order is visible in the district table, the order is not yet registered in the index
		DEBUG_WRITE(os_, CLASS_NAME, __func__,  "OUT_OF_RANGE Error in orderLineTable.getOrderLineMemoryAddress. Parameters: warehouseoffset: " << (int)warehouseOffset <<", dID: " << (int)dID << ", oID: "  <<(int)oID);
		res.isSuccessful = false;
		return;
	}

	res.isSuccessful = true;
	unsigned i = 0;
	for(auto id : distinctIDs) {
		res.itemIDs[i++] = id;
	}
	res.orderlinesCnt = distinctIDs.size();
}

void TPCC::TPCCDB::handleOldestUndeliveredOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::OldestUndeliveredOrderIndexResMsg &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::OLDEST_UNDELIVERED_ORDER;

	// first find the biggest orderID
	uint16_t warehouseOffset = req.parameters.oldestUndeliveredOrderIndex.warehouseOffset;
	uint8_t dID = req.parameters.oldestUndeliveredOrderIndex.dID;
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: Oldest_Undelivered_Order. Parameters: warehouseOffset = " << (int)warehouseOffset << ", dID = " << (int)dID);
	try{
		newOrderTable.getOldestNewOrder(warehouseOffset, dID, &res.oID, &res.clientWhoOrdered, &res.newOrderRegionOffset);
		orderTable.getOrderMemoryAddress(warehouseOffset, dID, res.oID, &res.clientWhoOrdered, &res.orderRegionOffset);
		orderLineTable.getOrderLineMemoryAddress(warehouseOffset, dID, res.oID, &res.clientWhoOrdered, &res.orderLineRegionOffset, &res.numOfOrderlines);
		res.isSuccessful = true;
		res.existUndeliveredOrder = true;
	}
	catch (const std::out_of_range& e) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] No undelivered order (wID: " << (int)warehouseOffset << ", dID: " << (int)dID << ")");
		res.isSuccessful = true;
		res.existUndeliveredOrder = false;
	}
}

void TPCC::TPCCDB::handleRegisterDeliveryIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::IndexResponseMessage &res){
	res.indexType = TPCC::IndexResponseMessage::IndexType::REGISTER_DELIVERY;

	// first find the biggest orderID
	uint16_t warehouseOffset = req.parameters.registerDeliveryIndex.warehouseOffset;
	uint8_t dID = req.parameters.registerDeliveryIndex.dID;
	uint32_t oID = req.parameters.registerDeliveryIndex.oID;
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)req.clientID
			<< ", Type: REGISTER_DELIVERY. Parameters: warehouseOffset = " << (int)warehouseOffset << ", dID = " << (int)dID << ", oID = " << (int)oID);
	try{
		newOrderTable.registerDelivery(warehouseOffset, dID, oID);
		res.isSuccessful = true;
	}
	catch (const std::out_of_range& e) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Error] cannot register delivery for order (wID: " << (int)warehouseOffset << ", dID: " << (int)dID << ", oID: " << oID << ")");
		res.isSuccessful = false;
	}
}


TPCC::TPCCDB::~TPCCDB(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called");
}

#endif /* SRC_BENCHMARKS_TPC_C_TPCCDB_CPP_ */
