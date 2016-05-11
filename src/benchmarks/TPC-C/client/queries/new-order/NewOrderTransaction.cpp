/*
 * NewOrderTransaction.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#include "NewOrderTransaction.hpp"
#include "../../../../../util/utils.hpp"

#define CLASS_NAME "NewOrderTrx"
namespace TPCC {

NewOrderTransaction::NewOrderTransaction(TPCCClient &client, DBExecutor &executor)
: BaseTransaction("New Order", client, executor),
  zipf(config::tpcc_settings::SKEWNESS_IN_ITEM_ACCESS, config::tpcc_settings::ITEMS_CNT){
//  zipf(3, config::tpcc_settings::ITEMS_CNT){

	localMemory_ 	= new NewOrderLocalMemory(os_, context_);
}

NewOrderTransaction::~NewOrderTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

void NewOrderTransaction::buildCart(){
	// reset cart
	cart_.reset();

	// 2.4.1.1 the home warehouse number (W_ID) is constant over the whole measurement interval
	cart_.wID = sessionState_.getHomeWarehouseID();

	// 2.4.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart_.dID = (uint8_t) random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.4.1.2 The non-uniform random customer number (C_ID) is selected using the NURand (1023,1,3000) function
	cart_.cID = (uint32_t) random_.NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);

	// 2.4.1.3 The number of items in the order (ol_cnt) is randomly selected within [5 .. 15] (an average of 10)
	uint8_t ol_cnt = (uint8_t ) random_.number(tpcc_settings::ORDER_MIN_OL_CNT, tpcc_settings::ORDER_MAX_OL_CNT);

	// 2.4.1.4 A fixed 1% of the New-Order transactions are chosen at random to simulate user data entry errors and exercise the performance of rolling back update transactions.
	// bool rollback = random_.number(1, 100) == 1;

	std::vector<uint32_t> uniqueItemIDs(ol_cnt);
	//std::set<uint32_t> uniqueIDSets = random_.selectUniqueNonUniformIds(8191, ol_cnt, 0, config::tpcc_settings::ITEMS_CNT - 1);
	std::set<uint32_t> uniqueIDSets = random_.selectZipfIds(ol_cnt, zipf);
	std::copy(uniqueIDSets.begin(), uniqueIDSets.end(), uniqueItemIDs.begin());

	for (int i = 0; i < ol_cnt; ++i) {
		NewOrderItem noi;
		// A non-uniform random item number (OL_I_ID) is selected using the NURand (8191,1,100000) function
		//noi.I_ID = (uint32_t) random_.NURand(8191, 0, config::tpcc_settings::ITEMS_CNT - 1);
		noi.I_ID = uniqueItemIDs.at(i);

		// TPC-C suggests generating a number in range (1, 100) and selecting remote on 1 (clause 2.4.1.5.2)
		// This provides more variation, and lets us tune the fraction of "remote" transactions.
		bool remote = ( ((double)random_.number(0, 100)) / 100) <= config::tpcc_settings::REMOTE_WAREHOUSE_PROB;
		if (config::tpcc_settings::WAREHOUSE_CNT > 1 && remote) {
			noi.OL_SUPPLY_W_ID = (uint16_t) random_.numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, sessionState_.getHomeWarehouseID());
		} else {
			noi.OL_SUPPLY_W_ID = sessionState_.getHomeWarehouseID();
		}

		// 2.4.1.5.3 A quantity (OL_QUANTITY) is randomly selected within [1 .. 10]
		noi.OL_QUANTITY = (uint8_t) random_.number(1, 10);
		cart_.items.push_back(noi);
	}
}

void NewOrderTransaction::initilizeTransaction(){
	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart_);
}


TPCC::TransactionResult NewOrderTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;
	TPCC::WarehouseVersion *warehouseV;
	TPCC::DistrictVersion *districtV;
	TPCC::CustomerVersion *customerV;
	std::vector<TPCC::ItemVersion*> items;
	std::vector<TPCC::StockVersion*> stocks;
	struct timespec beforeReadSnapshotTime, beforeIndexTime, afterExecutionTime, afterCheckVersionTime, afterLockTime, afterUpdateTime, afterCommitTime;


	if (config::SNAPSHOT_ACQUISITION_TYPE == config::SnapshotAcquisitionType::COMPLETE){
		// ************************************************
		//	Acquire read timestamp
		// ************************************************
		clock_gettime(CLOCK_REALTIME, &beforeReadSnapshotTime);
		executor_.getReadTimestamp(localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);
	}

	// ************************************************
	//	Read records in read-set
	// ************************************************
	// From Warehouse table, retrieve the row with matching W_ID.
	executor_.retrieveWarehouse(cart_.wID, *localMemory_->getWarehouseHead(), false);
	executor_.retrieveWarehousePointerList(cart_.wID, *localMemory_->getWarehouseTS(), false);
	warehouseV = localMemory_->getWarehouseHead()->getRegion();

	// From District table, retrieve the row with matching D_W_ID and D_ID.
	executor_.retrieveDistrict(cart_.wID, cart_.dID, *localMemory_->getDistrictHead(), false);
	executor_.retrieveDistrictPointerList(cart_.wID, cart_.dID, *localMemory_->getDistrictTS(), false);
	districtV = localMemory_->getDistrictHead()->getRegion();

	// From Customer table, retrieve C_DISCOUNT (the customer's discount rate), C_LAST (the customer's last name), and C_CREDIT (the customer's credit status)
	executor_.retrieveCustomer(cart_.wID, cart_.dID, cart_.cID, *localMemory_->getCustomerHead(), false);
	executor_.retrieveCustomerPointerList(cart_.wID, cart_.dID, cart_.cID, *localMemory_->getCustomerTS(), false);
	customerV = localMemory_->getCustomerHead()->getRegion();

	// Retrieve item and stock
	bool signaled = false;
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		// make the last request a signaled one
		if (olNumber == cart_.items.size() - 1) signaled = true;

		executor_.retrieveItem((size_t)olNumber, cart_.items.at(olNumber).I_ID, cart_.wID, *localMemory_->getItemHead(), signaled);
		items.push_back(&localMemory_->getItemHead()->getRegion()[olNumber]);

		executor_.retrieveStock((size_t)olNumber, cart_.items.at(olNumber).I_ID, cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockHead(), false);
		executor_.retrieveStockPointerList((size_t)olNumber, cart_.items.at(olNumber).I_ID, cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockTS(), true);
		stocks.push_back(&localMemory_->getStockHead()->getRegion()[olNumber]);
	}

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

		for (size_t i = 0; i < config::tpcc_settings::VERSION_NUM; i++){
			oracleContext_.insertClientIDIntoSnapshot(localMemory_->getWarehouseTS()->getRegion()[i].getClientID());
			oracleContext_.insertClientIDIntoSnapshot(localMemory_->getDistrictTS()->getRegion()[i].getClientID());
			oracleContext_.insertClientIDIntoSnapshot(localMemory_->getCustomerTS()->getRegion()[i].getClientID());
		}

		for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
			oracleContext_.insertClientIDIntoSnapshot(items[olNumber]->writeTimestamp.getClientID());
			oracleContext_.insertClientIDIntoSnapshot(stocks[olNumber]->writeTimestamp.getClientID());
		}


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
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle");
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Warehouse " << *warehouseV);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District " << *districtV);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Customer " << *customerV);
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Item " << *items[olNumber]);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved stock " << *stocks[olNumber] << " from warehouse " << (int)cart_.items.at(olNumber).OL_SUPPLY_W_ID);
		size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved the pointer list for Stock " << stocks[olNumber]->stock
				<< ": " << pointer_to_string(&localMemory_->getStockTS()->getRegion()[offset]) << " from warehouse " << (int)cart_.items.at(olNumber).OL_SUPPLY_W_ID );

		(void) offset; // to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false
	}


	clock_gettime(CLOCK_REALTIME, &afterExecutionTime);


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	// TODO: you don't need to send all requests signaled.

	bool abortFlag = false;
	int retrieveOlderVersionCnt = 0;
	if (! isRecordAccessible(warehouseV->writeTimestamp)){
		int ind = findValidVersion(localMemory_->getWarehouseTS()->getRegion(), config::tpcc_settings::VERSION_NUM);
		if (ind < 0) {
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Warehouse " << *warehouseV << " and none of its versions are not consistent (locked or from a later snapshot)");
		}
		else{
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Warehouse " << *warehouseV << " is not consistent, but its " << ind << "'s version is consistent (" << localMemory_->getWarehouseTS()->getRegion()[ind] << ")");
			executor_.retrieveWarehouseOlderVersion(cart_.wID, (size_t)ind, *localMemory_->getWarehouseHead(), true);
			retrieveOlderVersionCnt++;
		}
	}
	if (!abortFlag && ! isRecordAccessible(districtV->writeTimestamp)){
		int ind = findValidVersion(localMemory_->getDistrictTS()->getRegion(), config::tpcc_settings::VERSION_NUM);
		if (ind < 0) {
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << *districtV << " and none of its versions are not consistent (locked or from a later snapshot)");
		}
		else{
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << *districtV << " is not consistent, but its " << ind << "'s version is consistent (" << localMemory_->getDistrictTS()->getRegion()[ind] << ")");
			executor_.retrieveDistrictOlderVersion(cart_.wID, cart_.dID, (size_t)ind, *localMemory_->getDistrictHead(), true);
			retrieveOlderVersionCnt++;
		}
	}
	if (!abortFlag && ! isRecordAccessible(customerV->writeTimestamp)){
		int ind = findValidVersion(localMemory_->getCustomerTS()->getRegion(), config::tpcc_settings::VERSION_NUM);
		if (ind < 0) {
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Customer " << *customerV << " and none of its versions are not consistent (locked or from a later snapshot)");
		}
		else{
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Customer " << *customerV << " is not consistent, but its " << ind << "'s version is consistent (" << localMemory_->getCustomerTS()->getRegion()[ind] << ")");
			executor_.retrieveCustomerOlderVersion(cart_.wID, cart_.dID, cart_.cID, (size_t)ind, *localMemory_->getCustomerHead(), true);
			retrieveOlderVersionCnt++;
		}
	}
	if (!abortFlag){
		for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
			if (! isRecordAccessible(items[olNumber]->writeTimestamp)){
				abortFlag = true;
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": item " << *items[olNumber] << " is not consistent (locked or from a later snapshot)");
			}
			else if (! isRecordAccessible(stocks[olNumber]->writeTimestamp)){
				abortFlag = true;
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": stock " << *stocks[olNumber] << " is not consistent (locked or from a later snapshot)");
			}
		}
	}

	executor_.synchronizeSendEvents();

	// check if newly read records are accessible
	if (! isRecordAccessible(warehouseV->writeTimestamp) || ! isRecordAccessible(districtV->writeTimestamp) || ! isRecordAccessible(customerV->writeTimestamp)) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": the newly read records are also inconsistent"
				"(warehouse: " << warehouseV->writeTimestamp << ", district: " << districtV->writeTimestamp << ", customer: " << customerV->writeTimestamp << ")");
		abortFlag = true;
	}


	if (abortFlag == true) {
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "Client " << clientID_ << ": NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;
	}
	else DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": All received versions are consistent with READ snapshot, and all are unlocked");
	clock_gettime(CLOCK_REALTIME, &afterCheckVersionTime);


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
	primitive::version_offset_t	versionOffset;
	unsigned numberOfLocks = 0;


	if (! config::APPLY_COMMUTATIVE_UPDATES){
		// lock district
		versionOffset	= districtV->writeTimestamp.getVersionOffset();
		Timestamp districtLockTS (isDeleted, isLocked, versionOffset, clientID_, cts);

		executor_.lockDistrict(*districtV, districtLockTS, *localMemory_->getDistrictLockRegion(), true);
		numberOfLocks++;
	}

	// lock stocks
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		primitive::version_offset_t	versionOffset = stocks.at(olNumber)->writeTimestamp.getVersionOffset();
		primitive::client_id_t		clientID = clientID_;
		Timestamp lockTS (isDeleted, isLocked, versionOffset, clientID, cts);

		executor_.lockStock((size_t)olNumber, cart_.items.at(olNumber).I_ID, cart_.items.at(olNumber).OL_SUPPLY_W_ID, stocks.at(olNumber)->writeTimestamp, lockTS, *localMemory_->getStocksLocksRegion(), true);
		numberOfLocks++;
	}

	executor_.synchronizeSendEvents();

	// Byte swapping, since values read by atomic operations are in Big Endian order
	Timestamp districtExistingLock, districtExpectedLock;
	if (! config::APPLY_COMMUTATIVE_UPDATES){
		*localMemory_->getDistrictLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getDistrictLockRegion()->getRegion());
		districtExistingLock.copy(*localMemory_->getDistrictLockRegion()->getRegion());
		districtExpectedLock.copy(districtV->writeTimestamp);
	}

	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++)
		localMemory_->getStocksLocksRegion()->getRegion()[olNumber] = utils::bigEndianToHost(localMemory_->getStocksLocksRegion()->getRegion()[olNumber]);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": received the results for all the locks");

	// checks if locks are successful
	abortFlag = false;
	if (! config::APPLY_COMMUTATIVE_UPDATES){
		if (!districtExistingLock.isEqual(districtExpectedLock)) {
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock district " << districtV->district << " was NOT successful "
					<< "(expected: " << districtExpectedLock << ", existing: " << districtExistingLock << ")");
		}
	}

	std::vector<uint8_t> unsuccesfulOrderLines;
	std::vector<uint8_t> succesfulOrderLines;
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		Timestamp existingLock(localMemory_->getStocksLocksRegion()->getRegion()[olNumber]);
		Timestamp &expectedLock = stocks.at(olNumber)->writeTimestamp;
		if (! existingLock.isEqual(expectedLock)){
			unsuccesfulOrderLines.push_back(olNumber);
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock item " << (int)cart_.items.at(olNumber).I_ID << " was NOT successful "
					<< "(expected: " << expectedLock << ", existing: " << existingLock << ")");
		}
		else {
			succesfulOrderLines.push_back(olNumber);
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock item " << (int)cart_.items.at(olNumber).I_ID << " was successful "
					<< "(expected = existing = " << existingLock << ")");
		}
	}

	unsigned successfulLocksCnt = 0;

	if (abortFlag){
		// some locks couldn't get acquired. must first release the successful ones, then abort
		if (! config::APPLY_COMMUTATIVE_UPDATES){
			if (districtExistingLock.isEqual(districtExpectedLock)){
				successfulLocksCnt++;
				executor_.revertDistrictLock(*localMemory_->getDistrictHead(), true);
			}
		}

		for (auto const& olNumber: succesfulOrderLines){
			successfulLocksCnt++;
			executor_.revertStockLock((size_t)olNumber, cart_.items.at(olNumber).I_ID, cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockHead(), true);
		}

		executor_.synchronizeSendEvents();

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": could not acquire lock on all items --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
		return trxResult;
	}
	clock_gettime(CLOCK_REALTIME, &afterLockTime);


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
	if (! config::APPLY_COMMUTATIVE_UPDATES){
		executor_.updateDistrictPointers(*districtV, *localMemory_->getDistrictTS(), false);
		executor_.updateDistrictOlderVersions(*localMemory_->getDistrictHead(),	 false); 		// using DistrictHead instead of DistrictOlderVersions avoids redundant copying
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers and older versions for district " << districtV->district);
	}

	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		executor_.updateStockPointers((size_t)olNumber, stocks[olNumber], cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockTS(), false);
		executor_.updateStockOlderVersions((size_t)olNumber, stocks[olNumber], cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockOlderVersions(), false);
	}
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": added the pointers and older version");


	// ************************************************
	// Insert and unlock new records in write-set
	// ************************************************
	// update district (increment the next available order number D_NEXT_O_ID)
	isLocked = false;
	uint64_t old_D_NEXT_ID;
	uint64_t orderRID = BaseTransaction::getOrderRID();
	uint64_t newOrderRID = BaseTransaction::getNewOrderRID();
	uint64_t orderLineRID = BaseTransaction::reserveOrderLineRID(cart_.items.size());

	if (config::APPLY_COMMUTATIVE_UPDATES){
		executor_.retrieveAndIncrementDistrictNextOID(cart_.wID, cart_.dID, *localMemory_->getDistrictHead(), true);
		executor_.synchronizeSendEvents();

		localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID = utils::bigEndianToHost(localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID);
		old_D_NEXT_ID = localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID;
	}
	else {
		versionOffset = (primitive::version_offset_t)(districtV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
		districtV->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
		old_D_NEXT_ID = districtV->district.D_NEXT_O_ID;
		districtV->district.D_NEXT_O_ID = (uint64_t)districtV->district.D_NEXT_O_ID + 1;
		executor_.updateDistrict(*localMemory_->getDistrictHead(), true);
	}
	uint32_t assignedOID = (uint32_t)old_D_NEXT_ID;
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated district " << districtV->district << " with new D_NEXT_ID = " << (old_D_NEXT_ID + 1));


	// insert a new row into New-Order and Order table.
	// O_CARRIER_ID is set to a null value. If the order includes only home order-lines, then O_ALL_LOCAL is set to 1, otherwise O_ALL_LOCAL is set to 0.
	TPCC::OrderVersion *ov = localMemory_->getOrderHead()->getRegion();

	versionOffset = 0;
	ov->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	ov->order.O_ID = assignedOID;
	ov->order.O_W_ID = cart_.wID;
	ov->order.O_D_ID = cart_.dID;
	ov->order.O_C_ID = cart_.cID;
	ov->order.O_ENTRY_D = timer;
	ov->order.O_CARRIER_ID = 0;
	ov->order.O_OL_CNT = (uint8_t)cart_.items.size();
	ov->order.O_ALL_LOCAL = 1;	// TODO: must be fixed

	executor_.insertIntoOrder(clientID_, orderRID, cart_.wID, *localMemory_->getOrderHead(), false);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted Order " << *ov << ". Table position: " << orderRID);


	TPCC::NewOrderVersion *nov = localMemory_->getNewOrderHead()->getRegion();
	versionOffset = 0;
	nov->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
	nov->newOrder.NO_O_ID = assignedOID;
	nov->newOrder.NO_W_ID = cart_.wID;
	nov->newOrder.NO_D_ID = cart_.dID;

	executor_.insertIntoNewOrder(clientID_, newOrderRID, cart_.wID, *localMemory_->getNewOrderHead(), false);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted NewOrder " << *nov << ". Table position: " << newOrderRID);

	// For each O_OL_CNT item on the order:
	// |-- In Item table, I_PRICE, I_NAME and I_DATA are retrieved
	// |
	// |-- In Stock table, S_QUANTITY, the quantity in stock, S_DIST_xx, where xx represents the district number, and S_DATA are retrieved
	// |   If the retrieved value for S_QUANTITY exceeds OL_QUANTITY by 10 or more, then S_QUANTITY is decreased by OL_QUANTITY; otherwise
	// |   S_QUANTITY is updated to (S_QUANTITY - OL_QUANTITY)+91. S_YTD is increased by OL_QUANTITY and S_ORDER_CNT is incremented by 1.
	// |   If the order-line is remote, then S_REMOTE_CNT is incremented by 1.
	// |
	// |-- The strings in I_DATA and S_DATA are examined. If they both include the string "ORIGINAL",
	// |   the brand-generic field for that item is set to "B", otherwise, the brand-generic field is set to "G".
	// |
	// |-- A new row is inserted into the ORDER-LINE table to reflect the item on the order. OL_DELIVERY_D is set to a null value, OL_NUMBER is set to a unique value within
	// |   all the ORDER-LINE rows that have the same OL_O_ID value, and OL_DIST_INFO is set to the content of S_DIST_xx, where xx represents the district number (OL_D_ID)
	// |   The amount for the item in the order (OL_AMOUNT) is computed as: OL_QUANTITY * I_PRICE

	signaled = false;
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		bool remote = cart_.items.at(olNumber).OL_SUPPLY_W_ID == 0;	// TODO

		if (olNumber == cart_.items.size() - 1)
			signaled = true;

		versionOffset = (primitive::version_offset_t)(stocks[olNumber]->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
		stocks[olNumber]->writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
		if (stocks[olNumber]->stock.S_QUANTITY - cart_.items.at(olNumber).OL_QUANTITY >= 10)
			stocks[olNumber]->stock.S_QUANTITY = (uint16_t)(stocks[olNumber]->stock.S_QUANTITY - cart_.items.at(olNumber).OL_QUANTITY);
		else
			stocks[olNumber]->stock.S_QUANTITY = (uint16_t)(stocks[olNumber]->stock.S_QUANTITY - cart_.items.at(olNumber).OL_QUANTITY + 91);
		stocks[olNumber]->stock.S_YTD = (uint32_t)(stocks[olNumber]->stock.S_YTD + cart_.items.at(olNumber).OL_QUANTITY);
		stocks[olNumber]->stock.S_ORDER_CNT = (uint16_t)(stocks[olNumber]->stock.S_ORDER_CNT + 1);
		if (remote)
			stocks[olNumber]->stock.S_REMOTE_CNT++;

		executor_.updateStock((size_t)olNumber, stocks[olNumber], cart_.items.at(olNumber).OL_SUPPLY_W_ID, *localMemory_->getStockHead(), true);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated stock " << *stocks[olNumber] << " from warehouse " << (int)cart_.items.at(olNumber).OL_SUPPLY_W_ID);

		TPCC::OrderLineVersion &olv = localMemory_->getOrderLineHead()->getRegion()[olNumber];
		versionOffset = 0;
		olv.writeTimestamp.setAll(isDeleted, isLocked, versionOffset, clientID_, cts);
		olv.orderLine.OL_O_ID 			= assignedOID;
		olv.orderLine.OL_D_ID 			= cart_.dID;
		olv.orderLine.OL_W_ID 			= cart_.wID;
		olv.orderLine.OL_NUMBER 		= olNumber;
		olv.orderLine.OL_I_ID 			= items[olNumber]->item.I_ID;
		olv.orderLine.OL_SUPPLY_W_ID 	= cart_.items.at(olNumber).OL_SUPPLY_W_ID;
		olv.orderLine.OL_DELIVERY_D 	= (time_t)0;
		olv.orderLine.OL_QUANTITY 		=  cart_.items.at(olNumber).OL_QUANTITY;
		olv.orderLine.OL_AMOUNT 		= (float)cart_.items.at(olNumber).OL_QUANTITY * items[olNumber]->item.I_PRICE;
		std::memcpy(olv.orderLine.OL_DIST_INFO, stocks[olNumber]->stock.S_DIST[cart_.dID], 25);

		executor_.insertIntoOrderLine(clientID_, (uint64_t)(orderLineRID + olNumber), olNumber, cart_.wID, *localMemory_->getOrderLineHead(), signaled);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted OrderLine " << olv << ". Table position: " << orderLineRID + olNumber);
	}

	executor_.synchronizeSendEvents();

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully installed and unlocked all records");

	// update the index
	clock_gettime(CLOCK_REALTIME, &beforeIndexTime);
	size_t serverNum = Warehouse::getServerNum(cart_.wID);
	executor_.registerOrder(
			clientID_,
			cart_.wID,
			cart_.dID,
			cart_.cID,
			assignedOID,
			orderRID,
			newOrderRID,
			orderLineRID,
			(uint8_t)cart_.items.size(),
			true);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Msg sent. Type: REGISTER_ORDER. Params: wID = " << (int)cart_.wID
			<< ", dID = " << (int)cart_.dID << ", cID = " << (int)cart_.cID << ", oID = " << (int)assignedOID << ", regionOffset: " << orderRID << ", #orderlines: " << cart_.items.size());

	executor_.synchronizeNetworkEvents();

	assert(dsCtx_[serverNum]->getRegisterOrderIndexResponseMessage()->getRegion()->isSuccessful == true);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received for message . Type = REGISTER_ORDER");



	// ************************************************
	// Perform computation
	// ************************************************
	float sum = 0;
	for (uint8_t olNumber = 0; olNumber < cart_.items.size(); olNumber++){
		TPCC::OrderLineVersion &olv = localMemory_->getOrderLineHead()->getRegion()[olNumber];
		sum += olv.orderLine.OL_AMOUNT;
	}
	float totalAmount = sum * (1 - customerV->customer.C_DISCOUNT) * (1 + warehouseV->warehouse.W_TAX + districtV->district.D_TAX);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": totalAmount: " << totalAmount);
	(void)totalAmount;

	clock_gettime(CLOCK_REALTIME, &afterUpdateTime);


	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	trxResult.result = TransactionResult::Result::COMMITTED;
	trxResult.reason = TransactionResult::Reason::SUCCESS;
	trxResult.cts = cts;
	executor_.submitResult(clientID_, localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);

	executor_.synchronizeSendEvents();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << ": sent trx result for CTS " << cts << " to the Oracle");

	BaseTransaction::incrementOrderRID(1);
	BaseTransaction::incrementNewOrderRID(1);
	BaseTransaction::incrementOrderLineRID(cart_.items.size());

	clock_gettime(CLOCK_REALTIME, &afterCommitTime);

	// ************************************************
	//	Compute statistics
	// ************************************************
	trxResult.statistics.executionPhaseMicroSec = ( (double)( afterExecutionTime.tv_sec - beforeReadSnapshotTime.tv_sec ) * 1E9 + (double)( afterExecutionTime.tv_nsec - beforeReadSnapshotTime.tv_nsec ) ) / 1000;
	trxResult.statistics.checkVersionsPhaseMicroSec = ( (double)( afterCheckVersionTime.tv_sec - afterExecutionTime.tv_sec ) * 1E9 + (double)( afterCheckVersionTime.tv_nsec - afterExecutionTime.tv_nsec ) ) / 1000;
	trxResult.statistics.lockPhaseMicroSec = ( (double)( afterLockTime.tv_sec - afterCheckVersionTime.tv_sec ) * 1E9 + (double)( afterLockTime.tv_nsec - afterCheckVersionTime.tv_nsec ) ) / 1000;
	trxResult.statistics.updatePhaseMicroSec = ( (double)( afterUpdateTime.tv_sec - afterLockTime.tv_sec ) * 1E9 + (double)( afterUpdateTime.tv_nsec - afterLockTime.tv_nsec ) ) / 1000;
	trxResult.statistics.indexElapsedMicroSec = ( (double)( afterUpdateTime.tv_sec - beforeIndexTime.tv_sec ) * 1E9 + (double)( afterUpdateTime.tv_nsec - beforeIndexTime.tv_nsec ) ) / 1000;
	trxResult.statistics.commitSnapshotMicroSec = ( (double)( afterCommitTime.tv_sec - afterUpdateTime.tv_sec ) * 1E9 + (double)( afterCommitTime.tv_nsec - afterUpdateTime.tv_nsec ) ) / 1000;


	return trxResult;
}

} /* namespace TPCC */
