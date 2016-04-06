/*
 * TPCCIndices_Test.cpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#include "TPCCIndices_Test.hpp"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

TPCC::TPCCDB *TPCCIndices_Test::db;
RDMAContext *TPCCIndices_Test::context;

#define CLASS_NAME	"TPCCIndices_Test"

std::vector<std::function<void()>> TPCCIndices_Test::functionList_ {
	setup,
	test_getExistingCustomerByLastName,
	test_getNonExistingCustomerByLastName,
	test_clearOrderIndex,
	test_getExistingOrderAddressByOrderID,
	test_getNonExistingOrderAddressByOrderID,
	test_getLastOrderOfCustomer,
	test_getNonExistingLastOrderOfCustomer,
	cleanup
};

std::vector<std::function<void()>>& TPCCIndices_Test::getFunctionList() {
	return functionList_;
}

void TPCCIndices_Test::setup() {
	TestBase::printMessage(CLASS_NAME, __func__);

	srand ((unsigned int)time(NULL));

	size_t clientsCnt = 1;
	size_t warehouseTableSize = WAREHOUSE_PER_SERVER;
	size_t districtTableSize = DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t customerTableSize = CUSTOMER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t orderTableSize = clientsCnt * ORDER_PER_CLIENT;
	size_t orderLineTableSize = clientsCnt * tpcc_settings::ORDER_MAX_OL_CNT * ORDER_PER_CLIENT;
	size_t newOrderTableSize = clientsCnt * ORDER_PER_CLIENT;
	size_t stockTableSize = STOCK_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t itemTableSize = ITEMS_CNT;
	size_t historyTableSize = clientsCnt * TRANSACTION_CNT;
	size_t versionNum = VERSION_NUM;

	std::vector<uint16_t> warehouseIDs;
	for (uint16_t i = 0; i < warehouseTableSize; i++)
		warehouseIDs.push_back(i);

	context = new RDMAContext(config::IB_PORT[0]);

	TPCC::RealRandomGenerator random;
	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random);
	random.setC(cLoad);

	db = new TPCC::TPCCDB(
			warehouseIDs,
			warehouseTableSize,
			districtTableSize,
			customerTableSize,
			orderTableSize,
			orderLineTableSize,
			newOrderTableSize,
			stockTableSize,
			itemTableSize,
			historyTableSize,
			versionNum,
			random,
			*context);

}

void TPCCIndices_Test::cleanup() {
	TestBase::printMessage(CLASS_NAME, __func__);
	delete db;
	delete context;
}

void TPCCIndices_Test::test_getExistingCustomerByLastName(){
	TestBase::printMessage(CLASS_NAME, __func__);

	for (unsigned i = 0; i < 100000; i++){
		uint16_t wID = (uint16_t)(rand() % WAREHOUSE_PER_SERVER);
		uint8_t dID = (uint8_t)(rand() % DISTRICT_PER_WAREHOUSE);
		uint32_t cID = (uint32_t)(rand() % CUSTOMER_PER_DISTRICT);
		std::string lastName = std::string(db->customerTable.getCustomer(wID, dID, cID).C_LAST);

		try{
			uint32_t returnedCID = db->customerTable.getMiddleIDByLastName(wID, dID, lastName);
			assert (returnedCID < CUSTOMER_PER_DISTRICT);
			std::string returnedLastName = std::string(db->customerTable.getCustomer(wID, dID, returnedCID).C_LAST);
			assert(returnedLastName.compare(lastName) == 0);
		}
		catch (const std::exception& e){
			std::cerr << e.what() << std::endl;
			assert(1 == 2);
		}
	}
}

void TPCCIndices_Test::test_getNonExistingCustomerByLastName(){
	TestBase::printMessage(CLASS_NAME, __func__);

	uint16_t wID = 0;
	uint8_t dID = 0;
	std::string lastName = "Some_non_existing_familyname";

	try{
		db->customerTable.getMiddleIDByLastName(wID, dID, lastName);
		assert (1 == 2);
	}
	catch (const std::exception& e){
		assert(1 == 1);
	}
}


void TPCCIndices_Test::test_clearOrderIndex(){
	TestBase::printMessage(CLASS_NAME, __func__);

	uint16_t wID = 0;
	uint8_t dID = 1;
	uint32_t cID = 12;

	uint32_t oID = 123;
	primitive::client_id_t clientID = 1;
	size_t regionOffset = 1234;

	try{
		db->orderTable.registerNewOrderInIndex(wID, dID, cID, oID, clientID, regionOffset);
		db->orderTable.clearIndex();

		assert(db->orderTable.isIndexEmpty() == true);
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
	db->orderTable.clearIndex();
}

void TPCCIndices_Test::test_getExistingOrderAddressByOrderID(){
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		uint16_t wID = 0;
		uint8_t dID = 1;
		uint32_t cID = 12;
		uint32_t oID = 123;
		primitive::client_id_t clientID = 1;
		size_t regionOffset = 1234;
		db->orderTable.registerNewOrderInIndex(wID, dID, cID, oID, clientID, regionOffset);

		primitive::client_id_t returnedClientWhoOrdered;
		size_t returnedRegionOffset;

		db->orderTable.getOrderMemoryAddress(oID, &returnedClientWhoOrdered, &returnedRegionOffset);
		assert(returnedClientWhoOrdered == clientID);
		assert(returnedRegionOffset == regionOffset);

	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
	db->orderTable.clearIndex();

}

void TPCCIndices_Test::test_getNonExistingOrderAddressByOrderID(){
	TestBase::printMessage(CLASS_NAME, __func__);

	try{
		uint32_t oID = 123;
		primitive::client_id_t returnedClientWhoOrdered;
		size_t returnedRegionOffset;

		db->orderTable.getOrderMemoryAddress(oID, &returnedClientWhoOrdered, &returnedRegionOffset);
		assert(1 == 2);
	}
	catch (const std::exception& e){
		assert(1 == 1);
	}
	db->orderTable.clearIndex();

}
void TPCCIndices_Test::test_getLastOrderOfCustomer(){
	TestBase::printMessage(CLASS_NAME, __func__);

	uint16_t wID = 0;
	uint8_t dID = 1;
	uint32_t cID = 12;

	uint32_t oID_1 = 123;
	primitive::client_id_t clientID_1 = 1;
	size_t regionOffset_1 = 1234;

	uint32_t oID_2 = 234;
	primitive::client_id_t clientID_2 = 2;
	size_t regionOffset_2 = 2345;

	try{
		db->orderTable.registerNewOrderInIndex(wID, dID, cID, oID_1, clientID_1, regionOffset_1);
		db->orderTable.registerNewOrderInIndex(wID, dID, cID, oID_2, clientID_2, regionOffset_2);

		uint32_t biggestOID = db->orderTable.getBiggestOrderIDForCustomer(wID, dID, cID);
		assert (biggestOID == oID_2);
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
		assert(1 == 2);
	}
	db->orderTable.clearIndex();
}
void TPCCIndices_Test::test_getNonExistingLastOrderOfCustomer(){
	TestBase::printMessage(CLASS_NAME, __func__);

	uint16_t wID = 0;
	uint8_t dID = 1;
	uint32_t cID = 12;
	try{
		db->orderTable.getBiggestOrderIDForCustomer(wID, dID, cID);
		assert (1 == 2);
	}
	catch (const std::exception& e){
		assert(1 == 1);
	}
	db->orderTable.clearIndex();
}
