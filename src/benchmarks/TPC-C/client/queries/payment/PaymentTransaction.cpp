/*
 * PaymentTransaction.cpp
 *
 *  Created on: Mar 15, 2016
 *      Author: erfanz
 */

#include "PaymentTransaction.hpp"
#include "../../../../../util/utils.hpp"
#include <cstdio>	// for snprintf

#define CLASS_NAME "PaymentTrx"

namespace TPCC {


PaymentTransaction::PaymentTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: BaseTransaction("Payment", clientID, clientCnt, dsCtx, sessionState, random, context, oracleContext,localTimestampVector){
	localMemory_ 	= new PaymentLocalMemory(*context_);
}

PaymentTransaction::~PaymentTransaction() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}


TPCC::PaymentCart PaymentTransaction::buildCart(){
	PaymentCart cart;

	int x = random_->number(1, 100);
	int y = random_->number(1, 100);

	// 2.5.1.1 For any given terminal, the home warehouse number (W_ID) is constant over the whole measurement interval
	cart.wID = sessionState_->getHomeWarehouseID();

	// 2.5.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart.dID = (uint8_t) random_->number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.5.1.2 the customer resident warehouse is the home warehouse 85% of the time and is a randomly selected remote warehouse 15% of the time
	if (x <= 85 || config::tpcc_settings::WAREHOUSE_CNT == 1)
		cart.residentWarehouseID = cart.wID;
	else
		cart.residentWarehouseID = (uint16_t) random_->numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, cart.wID);

	// 2.5.1.2 The customer is randomly selected 60% of the time by last name (C_W_ID , C_D_ID, C_LAST) and 40% of the time by number
	if (y <= 60){
		// selection by LastName
		cart.customerSelectionMode = PaymentCart::LAST_NAME;
		random_->lastName(cart.cLastName, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
	}
	else {
		// selection by ID
		cart.customerSelectionMode = PaymentCart::ID;
		cart.cID = (uint32_t) random_->NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
	}

	// 2.5.1.3 The payment amount (H_AMOUNT) is random ly selected within [1.00 .. 5,000.00].
	cart.hAmount = random_->fixedPoint(2, 1.00, 5.00);

	return cart;
}

TPCC::TransactionResult PaymentTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;


	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	PaymentCart cart = buildCart();
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client " << (int)clientID_ << ": Cart: " << cart);


	// ************************************************
	//	Acquire read timestamp
	// ************************************************
	executor_.getReadTimestamp(*localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP());
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle");


	// ************************************************
	//	Read records in read-set
	// ************************************************
	// From Warehouse table, retrieve the row with matching W_ID.
	executor_.retrieveWarehouse(
			cart.wID,
			*localMemory_->getWarehouseHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);

	executor_.retrieveWarehousePointerList(
			cart.wID,
			*localMemory_->getWarehouseTS(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableTimestampList,
			getServerContext(cart.wID)->getQP(),
			false);
	TPCC::WarehouseVersion *warehouseV = localMemory_->getWarehouseHead()->getRegion();


	// From District table, retrieve the row with matching D_W_ID and D_ID.
	executor_.retrieveDistrict(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);

	executor_.retrieveDistrictPointerList(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictTS(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableTimestampList,
			getServerContext(cart.wID)->getQP(),
			true);
	TPCC::DistrictVersion *districtV = localMemory_->getDistrictHead()->getRegion();
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));


	if (cart.customerSelectionMode == PaymentCart::LAST_NAME) {
		// First, use the index on the server to find the cID for the given last name
		TPCC::ServerContext *serverCtx = getServerContext(cart.residentWarehouseID);

		executor_.lookupCustomerByLastName(
				clientID_,
				cart.residentWarehouseID,
				cart.dID,
				cart.cLastName,
				*serverCtx->getIndexRequestMessage(),
				*serverCtx->getIndexResponseMessage(),
				serverCtx->getQP(),
				true);

		DEBUG_COUT(CLASS_NAME, __func__, "[Send] Index Request Message sent. Type: LastName_TO_CID. Parameters: wID = " << (int)cart.residentWarehouseID << ", dID = " << (int)cart.dID << ", lastName = " << cart.cLastName);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

		TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));
		assert(serverCtx->getIndexResponseMessage()->getRegion()->isSuccessful == true);
		cart.cID = serverCtx->getIndexResponseMessage()->getRegion()->result.lastNameIndex.cID;
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] Index Response Message received. cID = " << (int)cart.cID);
	}

	// From Customer table, retrieve the row with matching C_W_ID (customer resident warehouse), C_D_ID and C_ID
	executor_.retrieveCustomer(
			cart.residentWarehouseID ,
			cart.dID,
			cart.cID,
			*localMemory_->getCustomerHead(),
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
			getServerContext(cart.residentWarehouseID)->getQP(),
			false);

	executor_.retrieveCustomerPointerList(
			cart.residentWarehouseID,
			cart.dID,
			cart.cID,
			*localMemory_->getDistrictTS(),
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList,
			getServerContext(cart.residentWarehouseID)->getQP(),
			true);

	TPCC::CustomerVersion *customerV = localMemory_->getCustomerHead()->getRegion();
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));


	// wait for all outstanding RDMA requests to finish
	//TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// printing for debugging purposes
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Warehouse " << warehouseV->warehouse);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District: " << districtV->district);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Customer " << customerV->customer);


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	bool abortFlag = false;
	if (! isRecordAccessible(warehouseV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Warehouse " << warehouseV->warehouse
				<< " (" << warehouseV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(districtV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << districtV->district
				<< " (" << districtV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(customerV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Customer " << customerV->customer
				<< " (" << customerV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}

	if (abortFlag == true) {
		DEBUG_COUT (CLASS_NAME, __func__, "Client " << clientID_ << ": NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;
	}
	else DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": All received versions are consistent with READ snapshot, and all are unlocked");


	// ************************************************
	//	Acquire Commit timestamp
	// ************************************************
	primitive::timestamp_t cts = getNewCommitTimestamp();
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": acquired commit timestamp " << cts);


	// ************************************************
	// Acquire locks for records in write-set
	// ************************************************
	primitive::lock_status_t 	lockStatus = 1;
	primitive::version_offset_t	versionOffset = warehouseV->writeTimestamp.getVersionOffset();
	primitive::client_id_t		clientID = clientID_;
	Timestamp warehouseLockTS (lockStatus, versionOffset, clientID, cts);

	executor_.lockWarehouse(
			*warehouseV,
			warehouseLockTS,
			*localMemory_->getWarehouseLockRegion(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	lockStatus = 1;
	versionOffset = districtV->writeTimestamp.getVersionOffset();
	clientID = clientID_;
	Timestamp districtLockTS (lockStatus, versionOffset, clientID, cts);

	executor_.lockDistrict(
			*districtV,
			districtLockTS,
			*localMemory_->getDistrictLockRegion(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP());


	lockStatus = 1;
	versionOffset = customerV->writeTimestamp.getVersionOffset();
	clientID = clientID_;
	Timestamp customerLockTS (lockStatus, versionOffset, clientID, cts);

	executor_.lockCustomer(
			*customerV,
			customerLockTS,
			*localMemory_->getCustomerLockRegion(),
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
			getServerContext(cart.residentWarehouseID)->getQP());


	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for warehouse lock
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for district lock
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for customer lock


	// Byte swapping, since values read by atomic operations are in Big Endian order
	*localMemory_->getWarehouseLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getWarehouseLockRegion()->getRegion());
	*localMemory_->getDistrictLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getDistrictLockRegion()->getRegion());
	*localMemory_->getCustomerLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getCustomerLockRegion()->getRegion());

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": received the results for all the locks");

	// checks if locks are successful
	Timestamp warehouseExistingLock(*localMemory_->getWarehouseLockRegion()->getRegion());
	Timestamp &warehouseExpectedLock = warehouseV->writeTimestamp;

	Timestamp districtExistingLock(*localMemory_->getDistrictLockRegion()->getRegion());
	Timestamp &districtExpectedLock = districtV->writeTimestamp;

	Timestamp customerExistingLock(*localMemory_->getCustomerLockRegion()->getRegion());
	Timestamp &customerExpectedLock = customerV->writeTimestamp;

	if (warehouseExistingLock.isEqual(warehouseExpectedLock)
			&& districtExistingLock.isEqual(districtExpectedLock)
			&& customerExistingLock.isEqual(customerExpectedLock))
		abortFlag = false;
	else
		abortFlag = true;

	if (abortFlag){
		unsigned successfulLockCnt = 0;
		if (! warehouseExistingLock.isEqual(warehouseExpectedLock)){
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock warehouse " << warehouseV->warehouse  << " was NOT successful "
					<< "(expected: " << warehouseExpectedLock << ", existing: " << warehouseExistingLock << ")");
		}
		else{
			executor_.revertWarehouseLock(
					*localMemory_->getWarehouseHead(),
					getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions,
					getServerContext(cart.wID)->getQP(),
					true);
			successfulLockCnt++;
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on warehouse " << warehouseV->warehouse);
		}
		if (! districtExistingLock.isEqual(districtExpectedLock)){
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock district " << districtV->district << " was NOT successful "
					<< "(expected: " << districtExpectedLock << ", existing: " << districtExistingLock << ")");
		}
		else{
			executor_.revertDistrictLock(
					*localMemory_->getDistrictHead(),
					getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
					getServerContext(cart.wID)->getQP(),
					true);
			successfulLockCnt++;
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on district " << districtV->district);
		}

		if (! customerExistingLock.isEqual(customerExpectedLock)){
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock customer" << customerV->customer << " was NOT successful "
					<< "(expected: " << customerExpectedLock << ", existing: " << customerExistingLock << ")");
		}
		else{
			executor_.revertCustomerLock(
					*localMemory_->getCustomerHead(),
					getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
					getServerContext(cart.residentWarehouseID)->getQP(),
					true);
			successfulLockCnt++;
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on customer " << customerV->customer);
		}

		for (unsigned i = 0; i < successfulLockCnt; i++){
			TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted lock");
		}

		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": could not acquire lock on all items --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
		return trxResult;

	}
	else{
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully acquired all locks");
	}


	// ************************************************
	//	Append old version to the versions list, and update the pointers list
	// ************************************************

	executor_.updateWarehousePointers(
			*warehouseV,
			*localMemory_->getWarehouseTS(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableTimestampList,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers for warehouse " << warehouseV->warehouse);

	executor_.updateWarehouseOlderVersions(
			//*localMemory_->getWarehouseOlderVersions(),
			*localMemory_->getWarehouseHead(),	// using WarehouseHead instead of WarehouseOlderVersions avoids redundant copying
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableOlderVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated older versions for warehouse " << warehouseV->warehouse);


	executor_.updateDistrictPointers(
			*districtV,
			*localMemory_->getDistrictTS(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableTimestampList,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers for district " << districtV->district);


	executor_.updateDistrictOlderVersions(
			//*localMemory_->getDistrictOlderVersions(),
			*localMemory_->getDistrictHead(),	// using DistrictHead instead of DistrictOlderVersions avoids redundant copying
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableOlderVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated older versions for district " << districtV->district);


	executor_.updateCustomerPointers(
			*customerV,
			*localMemory_->getCustomerTS(),
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList,
			getServerContext(cart.residentWarehouseID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers for customer" << customerV->customer);


	executor_.updateCustomerOlderVersions(
			//*localMemory_->getCustomerOlderVersions(),
			*localMemory_->getCustomerHead(),	// using CustomerHead instead of CustomerOlderVersions avoids redundant copying
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableOlderVersions,
			getServerContext(cart.residentWarehouseID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated older versions for customer" << customerV->customer);


	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": added the pointers and older versions");



	// ************************************************
	// Insert and unlock new records in write-set
	// ************************************************

	// update warehouse
	lockStatus = 0;
	versionOffset = (primitive::version_offset_t)(warehouseV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	warehouseV->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	warehouseV->warehouse.W_YTD += cart.hAmount;
	executor_.updateWarehouse(
			*localMemory_->getWarehouseHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated warehouse with wID: " << cart.wID );


	// update district
	versionOffset = (primitive::version_offset_t)(districtV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	districtV->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	districtV->district.D_YTD += cart.hAmount;
	executor_.updateDistrict(
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated district " << districtV->district);


	// update customer
	versionOffset = (primitive::version_offset_t)(customerV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	customerV->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	customerV->customer.C_BALANCE -= cart.hAmount;
	customerV->customer.C_YTD_PAYMENT += cart.hAmount;
	customerV->customer.C_PAYMENT_CNT = (uint16_t)(customerV->customer.C_PAYMENT_CNT + 1);
	if (strcmp(customerV->customer.C_CREDIT, "BC")){
		// Bad credit: insert history into c_data
		char history[501];
		int characters = snprintf(history, 501, "(%d, %d, %d, %d, %d, %.2f)\n",
				cart.cID, cart.dID, cart.residentWarehouseID, cart.dID, cart.wID, cart.hAmount);
		assert(characters < 501);

		// Perform the insert with a move and copy
		int current_keep = static_cast<int>(strlen(customerV->customer.C_DATA));
		if (current_keep + characters > 500) {
			current_keep = 500 - characters;
		}
		assert(current_keep + characters <= 500);
		memmove(customerV->customer.C_DATA + characters, customerV->customer.C_DATA, current_keep);
		memcpy(customerV->customer.C_DATA, history, characters);
		customerV->customer.C_DATA[characters + current_keep] = '\0';
		assert(strlen(customerV->customer.C_DATA) == (size_t)(characters + current_keep));
	}
	executor_.updateCustomer(
			*localMemory_->getCustomerHead(),
			getServerContext(cart.residentWarehouseID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
			getServerContext(cart.residentWarehouseID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated customer " << customerV->customer);


	// insert new history
	TPCC::HistoryVersion *historyV = localMemory_->getHistoryHead()->getRegion();
	versionOffset = 0;
	historyV->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	historyV->history.H_D_ID	= cart.dID;
	historyV->history.H_C_W_ID	= cart.residentWarehouseID;
	historyV->history.H_C_D_ID	= cart.dID;
	historyV->history.H_C_ID	= cart.cID;
	historyV->history.H_C_ID	= cart.wID;
	historyV->history.H_AMOUNT	= cart.hAmount;
	historyV->history.H_DATE	= timer;
	strcpy(historyV->history.H_DATA, warehouseV->warehouse.W_NAME);
	strcat(historyV->history.H_DATA, "    ");
	strcat(historyV->history.H_DATA, districtV->district.D_NAME);
	executor_.insertIntoHistory(
			clientID_,
			nextHistoryID_,
			*localMemory_->getHistoryHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->historyTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			true);
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted history with hID = " << (int)nextHistoryID_);
	nextHistoryID_++;

	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	trxResult.result = TransactionResult::Result::COMMITTED;
	trxResult.reason = TransactionResult::Reason::SUCCESS;
	trxResult.cts = cts;
	localTimestampVector_->getRegion()[clientID_] = trxResult.cts;
	executor_.submitResult(clientID_, *localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP());
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << ": sent trx result for CTS " << cts << " to the Oracle");

	return trxResult;

}

} /* namespace TPCC */
