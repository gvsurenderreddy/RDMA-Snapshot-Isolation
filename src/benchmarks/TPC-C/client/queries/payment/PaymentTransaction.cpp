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

PaymentTransaction::PaymentTransaction(TPCCClient &client, DBExecutor &executor)
: BaseTransaction("Payment", client, executor){
	localMemory_ 	= new PaymentLocalMemory(os_, context_);
}

PaymentTransaction::~PaymentTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

void PaymentTransaction::buildCart(){
	cart_.reset();

	int x = random_.number(1, 100);
	int y = random_.number(1, 100);

	// 2.5.1.1 For any given terminal, the home warehouse number (W_ID) is constant over the whole measurement interval
	cart_.wID = sessionState_.getHomeWarehouseID();

	// 2.5.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart_.dID = (uint8_t) random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.5.1.2 the customer resident warehouse is the home warehouse 85% of the time and is a randomly selected remote warehouse 15% of the time
	if (x <= 85 || config::tpcc_settings::WAREHOUSE_CNT == 1)
		cart_.residentWarehouseID = cart_.wID;
	else
		cart_.residentWarehouseID = (uint16_t) random_.numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, cart_.wID);

	// 2.5.1.2 The customer is randomly selected 60% of the time by last name (C_W_ID , C_D_ID, C_LAST) and 40% of the time by number
	if (y <= 60){
		// selection by LastName
		cart_.customerSelectionMode = PaymentCart::LAST_NAME;
		random_.lastName(cart_.cLastName, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
	}
	else {
		// selection by ID
		cart_.customerSelectionMode = PaymentCart::ID;
		cart_.cID = (uint32_t) random_.NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
	}

	// 2.5.1.3 The payment amount (H_AMOUNT) is random ly selected within [1.00 .. 5,000.00].
	cart_.hAmount = random_.fixedPoint(2, 1.00, 5.00);
}

void PaymentTransaction::initilizeTransaction(){
	// ************************************************
	//	Constructing the transaction cart
	// ************************************************
	buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart_);
}

TPCC::TransactionResult PaymentTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;

	if (config::SNAPSHOT_ACQUISITION_TYPE == config::SnapshotAcquisitionType::COMPLETE){
		// ************************************************
		//	Acquire read timestamp
		// ************************************************
		executor_.getReadTimestamp(localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle");
	}

	// ************************************************
	//	Read records in read-set
	// ************************************************
	// From Warehouse table, retrieve the row with matching W_ID.
	executor_.retrieveWarehouse(cart_.wID, *localMemory_->getWarehouseHead(), false);
	executor_.retrieveWarehousePointerList(cart_.wID, *localMemory_->getWarehouseTS(), false);
	TPCC::WarehouseVersion *warehouseV = localMemory_->getWarehouseHead()->getRegion();

	// From District table, retrieve the row with matching D_W_ID and D_ID.
	executor_.retrieveDistrict(cart_.wID, cart_.dID, *localMemory_->getDistrictHead(), false);
	executor_.retrieveDistrictPointerList(cart_.wID, cart_.dID, *localMemory_->getDistrictTS(), true);
	TPCC::DistrictVersion *districtV = localMemory_->getDistrictHead()->getRegion();

	executor_.synchronizeSendEvents();


	if (cart_.customerSelectionMode == PaymentCart::LAST_NAME) {
		// First, use the index on the server to find the cID for the given last name
		size_t serverNum = Warehouse::getServerNum(cart_.wID);

		executor_.lookupCustomerByLastName(
				clientID_,
				cart_.residentWarehouseID,
				cart_.dID,
				cart_.cLastName,
				*dsCtx_[serverNum]->getIndexRequestMessage(),
				*dsCtx_[serverNum]->getCustomerNameIndexResponseMessage(),
				dsCtx_[serverNum]->getQP(),
				true);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: LastName_TO_CID. Parameters: wID = " << (int)cart_.residentWarehouseID << ", dID = " << (int)cart_.dID << ", lastName = " << cart_.cLastName);

		executor_.synchronizeNetworkEvents();

		if (dsCtx_[serverNum]->getCustomerNameIndexResponseMessage()->getRegion()->isSuccessful == false){
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. Customer has no register order. Therefore, COMMITs without any further action.");
			trxResult.result = TransactionResult::Result::COMMITTED;
			return trxResult;
		}
		cart_.cID = dsCtx_[serverNum]->getCustomerNameIndexResponseMessage()->getRegion()->cID;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. cID = " << (int)cart_.cID);
	}

	// From Customer table, retrieve the row with matching C_W_ID (customer resident warehouse), C_D_ID and C_ID
	executor_.retrieveCustomer(cart_.residentWarehouseID, cart_.dID, cart_.cID, *localMemory_->getCustomerHead(), false);
	executor_.retrieveCustomerPointerList(cart_.residentWarehouseID, cart_.dID, cart_.cID, *localMemory_->getDistrictTS(), true);
	TPCC::CustomerVersion *customerV = localMemory_->getCustomerHead()->getRegion();

	// wait for all outstanding RDMA requests to finish
	executor_.synchronizeSendEvents();

	if (config::SNAPSHOT_ACQUISITION_TYPE == config::SnapshotAcquisitionType::ONLY_READ_SET){
		// ************************************************
		//	Acquire Partial read timestamp
		// ************************************************

		// First find which clients are needed to put in the snapshot
		oracleContext_.insertClientIDIntoSnapshot(warehouseV->writeTimestamp.getClientID());
		oracleContext_.insertClientIDIntoSnapshot(districtV->writeTimestamp.getClientID());
		oracleContext_.insertClientIDIntoSnapshot(customerV->writeTimestamp.getClientID());

		// then, construct the snapshot, only limited to those clients
		// decide if it makes more sense to get the partial snapshot through multiple messages or  get the entire snapshot through one message
		if (oracleContext_.getClientIDsInSnapshot().size() > (clientCnt_ / 10)) {
			// get the entire snapshot
			executor_.getReadTimestamp(localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);
		}
		else{
			executor_.getPartialSnapshot(
					localTimestampVector_,
					oracleContext_.getClientIDsInSnapshot(),
					oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector,
					oracleContext_.getQP(),
					true);
		}
	}
	executor_.synchronizeSendEvents();

	// printing for debugging purposes
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Warehouse " << warehouseV->warehouse);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District: " << districtV->district);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Customer " << customerV->customer);


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	bool abortFlag = false;
	if (! isRecordAccessible(warehouseV->writeTimestamp)){
		abortFlag = true;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Warehouse " << warehouseV->warehouse
				<< " (" << warehouseV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(districtV->writeTimestamp)){
		abortFlag = true;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << districtV->district
				<< " (" << districtV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(customerV->writeTimestamp)){
		abortFlag = true;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Customer " << customerV->customer
				<< " (" << customerV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}

	if (abortFlag == true) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "Client " << clientID_ << ": NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;
	}
	else DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": All received versions are consistent with READ snapshot, and all are unlocked");


	// ************************************************
	//	Acquire Commit timestamp
	// ************************************************
	primitive::timestamp_t cts = getNewCommitTimestamp();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": acquired commit timestamp " << cts);


	// ************************************************
	// Acquire locks for records in write-set
	// ************************************************
	bool isLocked = true;
	bool isDeleted = false;
	primitive::version_offset_t	versionOffset = warehouseV->writeTimestamp.getVersionOffset();
	primitive::client_id_t		clientID = clientID_;
	Timestamp warehouseLockTS (isDeleted, isLocked, versionOffset, clientID, cts);
	executor_.lockWarehouse(*warehouseV, warehouseLockTS, *localMemory_->getWarehouseLockRegion(), false);

	versionOffset = districtV->writeTimestamp.getVersionOffset();
	clientID = clientID_;
	Timestamp districtLockTS (isDeleted, isLocked, versionOffset, clientID, cts);
	executor_.lockDistrict(*districtV, districtLockTS, *localMemory_->getDistrictLockRegion(), true);

	versionOffset = customerV->writeTimestamp.getVersionOffset();
	clientID = clientID_;
	Timestamp customerLockTS (isDeleted, isLocked, versionOffset, clientID, cts);
	executor_.lockCustomer(*customerV, customerLockTS, *localMemory_->getCustomerLockRegion(), true);

	executor_.synchronizeSendEvents();

	// Byte swapping, since values read by atomic operations are in Big Endian order
	*localMemory_->getWarehouseLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getWarehouseLockRegion()->getRegion());
	*localMemory_->getDistrictLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getDistrictLockRegion()->getRegion());
	*localMemory_->getCustomerLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getCustomerLockRegion()->getRegion());

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": received the results for all the locks");

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
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock warehouse " << warehouseV->warehouse  << " was NOT successful "
					<< "(expected: " << warehouseExpectedLock << ", existing: " << warehouseExistingLock << ")");
		}
		else{
			executor_.revertWarehouseLock(*localMemory_->getWarehouseHead(), true);
			successfulLockCnt++;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on warehouse " << warehouseV->warehouse);
		}
		if (! districtExistingLock.isEqual(districtExpectedLock)){
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock district " << districtV->district << " was NOT successful "
					<< "(expected: " << districtExpectedLock << ", existing: " << districtExistingLock << ")");
		}
		else{
			executor_.revertDistrictLock(*localMemory_->getDistrictHead(), true);
			successfulLockCnt++;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on district " << districtV->district);
		}

		if (! customerExistingLock.isEqual(customerExpectedLock)){
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock customer" << customerV->customer << " was NOT successful "
					<< "(expected: " << customerExpectedLock << ", existing: " << customerExistingLock << ")");
		}
		else{
			executor_.revertCustomerLock(*localMemory_->getCustomerHead(), true);
			successfulLockCnt++;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted the successful lock on customer " << customerV->customer);
		}

		executor_.synchronizeSendEvents();

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": could not acquire lock on all items --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
		return trxResult;

	}
	else DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully acquired all locks");


	// ************************************************
	//	Write the command to logs
	// ************************************************
	// At this point, it is determined that the transaction can successfully commit
	localTimestampVector_.getRegion()[clientID_] = cts;
	if (config::recovery_settings::RECOVERY_ENABLED){
		char command[config::recovery_settings::COMMAND_LOG_SIZE];
		size_t messageSize = cart_.logMessage(command);
		recoveryClient_.writeCommandToLog(localTimestampVector_, oracleContext_.getClientIDsInSnapshot(), command, messageSize);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Command written to log");
	}


	// ************************************************
	//	Append old version to the versions list, and update the pointers list
	// ************************************************
	executor_.updateWarehousePointers(*warehouseV, *localMemory_->getWarehouseTS(), false);
	executor_.updateWarehouseOlderVersions(*localMemory_->getWarehouseHead(), false);	// using WarehouseHead instead of WarehouseOlderVersions avoids redundant copying
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers and older versions for warehouse " << warehouseV->warehouse);

	executor_.updateDistrictPointers(*districtV, *localMemory_->getDistrictTS(), false);
	executor_.updateDistrictOlderVersions(*localMemory_->getDistrictHead(),	 false);	// using DistrictHead instead of DistrictOlderVersions avoids redundant copying
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers and older versions for district " << districtV->district);

	executor_.updateCustomerPointers(*customerV, *localMemory_->getCustomerTS(), false);
	executor_.updateCustomerOlderVersions(*localMemory_->getCustomerHead(),	false);	// using CustomerHead instead of CustomerOlderVersions avoids redundant copying
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers and older versions for customer" << customerV->customer);


	// ************************************************
	// Insert and unlock new records in write-set
	// ************************************************
	// update warehouse
	isLocked = false;
	versionOffset = (primitive::version_offset_t)(warehouseV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	warehouseV->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	warehouseV->warehouse.W_YTD += cart_.hAmount;
	executor_.updateWarehouse(*localMemory_->getWarehouseHead(), false);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated warehouse with wID: " << cart_.wID );

	// update district
	versionOffset = (primitive::version_offset_t)(districtV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	districtV->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	districtV->district.D_YTD += cart_.hAmount;
	executor_.updateDistrict(*localMemory_->getDistrictHead(), false);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated district " << districtV->district);

	// update customer
	versionOffset = (primitive::version_offset_t)(customerV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	customerV->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	customerV->customer.C_BALANCE -= cart_.hAmount;
	customerV->customer.C_YTD_PAYMENT += cart_.hAmount;
	customerV->customer.C_PAYMENT_CNT = (uint16_t)(customerV->customer.C_PAYMENT_CNT + 1);
	if (strcmp(customerV->customer.C_CREDIT, "BC")){
		// Bad credit: insert history into c_data
		char history[501];
		int characters = snprintf(history, 501, "(%d, %d, %d, %d, %d, %.2f)\n",
				cart_.cID, cart_.dID, cart_.residentWarehouseID, cart_.dID, cart_.wID, cart_.hAmount);
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
	executor_.updateCustomer(*localMemory_->getCustomerHead(), false);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated customer " << customerV->customer);


	// insert new history
	TPCC::HistoryVersion *historyV = localMemory_->getHistoryHead()->getRegion();
	versionOffset = 0;
	historyV->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	historyV->history.H_D_ID	= cart_.dID;
	historyV->history.H_C_W_ID	= cart_.residentWarehouseID;
	historyV->history.H_C_D_ID	= cart_.dID;
	historyV->history.H_C_ID	= cart_.cID;
	historyV->history.H_C_ID	= cart_.wID;
	historyV->history.H_AMOUNT	= cart_.hAmount;
	historyV->history.H_DATE	= timer;
	strcpy(historyV->history.H_DATA, warehouseV->warehouse.W_NAME);
	strcat(historyV->history.H_DATA, "    ");
	strcat(historyV->history.H_DATA, districtV->district.D_NAME);
	executor_.insertIntoHistory(clientID_, BaseTransaction::getHistoryRID(), *localMemory_->getHistoryHead(), true);

	executor_.synchronizeSendEvents();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted history with hID = " << (int)BaseTransaction::getHistoryRID());
	BaseTransaction::incrementHistoryRID(1);


	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	trxResult.result = TransactionResult::Result::COMMITTED;
	trxResult.reason = TransactionResult::Reason::SUCCESS;
	trxResult.cts = cts;
	executor_.submitResult(clientID_, localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);

	executor_.synchronizeSendEvents();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << ": sent trx result for CTS " << cts << " to the Oracle");

	return trxResult;
}

} /* namespace TPCC */
