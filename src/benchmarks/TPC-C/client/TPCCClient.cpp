/*
 * TPCCClient.cpp
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "TPCCClient.hpp"
#include "../../../util/RDMACommon.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include "../tables/WarehouseTable.hpp"
#include <infiniband/verbs.h>
#include <string>
#include <vector>
#include <time.h>		// for struct timespec




#define CLASS_NAME "TPCCClient"

TPCC::TPCCClient::TPCCClient(unsigned instanceNum, uint16_t homeWarehouseID, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort){
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed
	//zipf_initialize(config::SKEWNESS_IN_ITEM_ACCESS, config::ITEM_PER_SERVER);
	// nextEpoch_ = (primitive::timestamp_t) 1ULL;

	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random_);
	random_.setC(cLoad);

	context_ 		= new RDMAContext(ibPort_);
	localMemory_ 	= new NewOrderLocalMemory(*context_);
	sessionState_ 	= new SessionState(homeWarehouseID, (primitive::timestamp_t) 1ULL);

	// ************************************************
	//	Connect to Oracle
	// ************************************************
	int sockfd;
	TEST_NZ (utils::establish_tcp_connection(config::TIMESTAMP_SERVER_ADDR, config::TIMESTAMP_SERVER_PORT, &sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Oracle");
	oracleContext_ = new OracleContext(sockfd, config::TIMESTAMP_SERVER_PORT, config::TIMESTAMP_SERVER_IB_PORT, *context_);

	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(
			oracleContext_->getQP(),
			oracleContext_->getRemoteMemoryKeys()->getRDMAHandler(),
			(uintptr_t)oracleContext_->getRemoteMemoryKeys()->getRegion(),
			(uint32_t)oracleContext_->getRemoteMemoryKeys()->getRegionSizeInByte());


	oracleContext_->activateQueuePair(*context_);
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to Oracle");

	TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from Oracle");

	// Set client ID and client cnt
	clientID_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_id;
	clientCnt_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_cnt;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is assigned ID = " << clientID_ << " out of " << clientCnt_ << " clients");

	localTimestampVector_	= new RDMARegion<primitive::timestamp_t>(clientCnt_, *context_, IBV_ACCESS_LOCAL_WRITE);



	// ************************************************
	//	Connect to Servers
	// ************************************************
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::establish_tcp_connection(config::SERVER_ADDR[i], config::TCP_PORT[i], &sockfd));

		// build server context
		dsCtx_.push_back(new ServerContext(sockfd, config::SERVER_ADDR[i], config::TCP_PORT[i], config::IB_PORT[i], instanceNum, *context_));

		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server " << i);


		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getRemoteMemoryKeys()->getRDMAHandler(),
				(uintptr_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegion(),
				(uint32_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegionSizeInByte());

		dsCtx_[i]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server " << i);

		TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from server " << i);
	}

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Starting transactions ");

	int abortCnt = 0;
	int committedCnt = 0;
	int abortDueToInconsistentSnapshot = 0;
	int abortDueToUnsuccessfulLock = 0;
	struct timespec firstRequestTime, lastRequestTime;

	clock_gettime(CLOCK_REALTIME, &firstRequestTime);

	for (int i=0; i < config::tpcc_settings::NEWORDER_TRANSACTION_CNT; i++){
		DEBUG_COUT(CLASS_NAME, __func__, "--------------- [Info] Transaction " << i << ": --------------");
		TransactionResult trxResult = doNewOrder();
		if (trxResult.result == TransactionResult::Result::ABORTED){
			abortCnt++;
			if (trxResult.reason == TransactionResult::Reason::INCONSISTENT_SNAPSHOT)
				abortDueToInconsistentSnapshot++;
			else if (trxResult.reason == TransactionResult::Reason::UNSUCCESSFUL_LOCK)
				abortDueToUnsuccessfulLock++;
		}
		else committedCnt++;
	}

	clock_gettime(CLOCK_REALTIME, &lastRequestTime);

	double microElapsedTime = ( (double)( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + (double)( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;


	double success_rate = (double)committedCnt / config::tpcc_settings::NEWORDER_TRANSACTION_CNT;
	double inconsistentSnapshotRatio = (abortCnt==0) ? 0 : (double)abortDueToInconsistentSnapshot/abortCnt;
	double unsuccessfulLockRatio = (abortCnt==0) ? 0 : (double)abortDueToUnsuccessfulLock/abortCnt;
	double trxsPerSec = (double)(committedCnt / (double)(microElapsedTime / (1000 * 1000) ));


	std::cout << "[Stat] " << committedCnt << " committed, " << abortCnt << " aborted. success rate:	" << success_rate << std::endl;
	std::cout << "[Stat] Avg abort type I (snapshot) ratio	" << inconsistentSnapshotRatio << std::endl;
	std::cout << "[Stat] Avg abort type II (lock) ratio	" << unsuccessfulLockRatio << std::endl;
	std::cout << "[Stat] Committed Transactions/sec:	" <<  trxsPerSec << std::endl;

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::sock_sync (dsCtx_[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified server " << i << " it's done");
		delete dsCtx_[i];
	}

	TEST_NZ (utils::sock_sync (oracleContext_->getSockFd()));	// just send a dummy char back and forth
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified Oracle it's done");
}

bool TPCC::TPCCClient::isRecordAccessible(Timestamp &ts){
	if (ts.isLocked())
		// item is already locked
		return false;

	primitive::client_id_t committingClient = ts.getClientID();
	if (committingClient == clientID_)
		// regardless of whether the version matches the snapshot or not, the version is installed by the client itself, so is valid
		// when is it useful? when using adaptive abort rate control.
		return true;
	if (ts.getTimestamp() > localTimestampVector_->getRegion()[committingClient])
		// from a later snapshot, so not useful
		return false;

	return true;
}

TPCC::Cart TPCC::TPCCClient::buildCart(){
	Cart cart;

	// 2.4.1.1 the home warehouse number (W_ID) is constant over the whole measurement interval
	// cart.wID = (uint16_t) random_.number(0, config::tpcc_settings::WAREHOUSE_CNT - 1);
	cart.wID = sessionState_->getHomeWarehouseID();

	// 2.4.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart.dID = (uint8_t) random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.4.1.2 The non-uniform random customer number (C_ID) is selected using the NURand (1023,1,3000) function
	cart.cID = (uint32_t) random_.NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);

	// 2.4.1.3 The number of items in the order (ol_cnt) is randomly selected within [5 .. 15] (an average of 10)
	uint8_t ol_cnt = (uint8_t ) random_.number(tpcc_settings::ORDER_MIN_OL_CNT, tpcc_settings::ORDER_MAX_OL_CNT);


	// 2.4.1.4 A fixed 1% of the New-Order transactions are chosen at random to simulate user data entry errors and exercise the performance of rolling back update transactions.
	// bool rollback = random_.number(1, 100) == 1;

	std::vector<uint32_t> uniqueItemIDs(ol_cnt);
	std::set<uint32_t> uniqueIDSets = random_.selectUniqueNonUniformIds(8191, ol_cnt, 0, config::tpcc_settings::ITEMS_CNT - 1);
	std::copy(uniqueIDSets.begin(), uniqueIDSets.end(), uniqueItemIDs.begin());

	for (int i = 0; i < ol_cnt; ++i) {
		NewOrderItem noi;
		// A non-uniform random item number (OL_I_ID) is selected using the NURand (8191,1,100000) function
		//noi.I_ID = (uint32_t) random_.NURand(8191, 0, config::tpcc_settings::ITEMS_CNT - 1);
		noi.I_ID = uniqueItemIDs.at(i);

		// TPC-C suggests generating a number in range (1, 100) and selecting remote on 1 (clause 2.4.1.5.2)
		// This provides more variation, and lets us tune the fraction of "remote" transactions.
		bool remote = random_.number(0, 1) <= config::tpcc_settings::REMOTE_WAREHOUSE_PROB;
		if (config::tpcc_settings::WAREHOUSE_CNT > 1 && remote) {
			noi.OL_SUPPLY_W_ID = (uint16_t) random_.numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, sessionState_->getHomeWarehouseID());
		} else {
			noi.OL_SUPPLY_W_ID = sessionState_->getHomeWarehouseID();
		}

		// 2.4.1.5.3 A quantity (OL_QUANTITY) is randomly selected within [1 .. 10]
		noi.OL_QUANTITY = (uint8_t) random_.number(1, 10);
		cart.items.push_back(noi);
	}
	return cart;
}




TPCC::TransactionResult TPCC::TPCCClient::doNewOrder(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;


	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	Cart cart = buildCart();
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Cart: " << cart);


	// ************************************************
	//	Acquire read timestamp
	// ************************************************
	getReadTimestamp();
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " received read snapshot from oracle");


	// ************************************************
	//	Read records in read-set
	// ************************************************
	// From Warehouse table, retrieve W_TAX
	retrieveWarehouseTax(cart.wID);

	// From District table, retrieve D_TAX
	retrieveDistrictTax(cart.wID, cart.dID);

	// From Customer table, retrieve C_DISCOUNT (the customer's discount rate), C_LAST (the customer's last name), and C_CREDIT (the customer's credit status)
	TPCC::CustomerVersion *customerV = getCustomerInformation(cart.wID, cart.dID, cart.cID);

	std::vector<TPCC::ItemVersion*> items;
	std::vector<TPCC::StockVersion*> stocks;

	// Retrieve item and stock
	bool signaled = false;
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		ItemVersion* itemV = retrieveItem(olNumber, cart.items.at(olNumber).I_ID, cart.wID);
		items.push_back(itemV);

		StockVersion* stockV = retrieveStock(olNumber, cart.items.at(olNumber).I_ID, cart.items.at(olNumber).OL_SUPPLY_W_ID);
		stocks.push_back(stockV);

		if (olNumber == cart.items.size() - 1)
			// make the last request a signaled one
			signaled = true;
		retrieveStockPointerList(olNumber, cart.items.at(olNumber).I_ID, cart.items.at(olNumber).OL_SUPPLY_W_ID, signaled);
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Warehouse " << (int)cart.wID);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved District D_W_ID: " << (int)cart.wID << ", D_ID: " << (int)cart.dID);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Customer " << customerV->customer);
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Item: " << items[olNumber]->item);
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved stock: " << *stocks[olNumber] << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);

		size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved the pointer list for Stock " << (int)stocks[olNumber]->stock.S_I_ID
				<< ": " << pointer_to_string(&localMemory_->getStockTS()->getRegion()[offset]) << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID );
	}


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	// IMPORTANT: This step has to be done AFTER acquiring CTS. It follows that if the snapshot is already overwritten, the transaction has to ABORT and
	// flip the bit on the TS server.
	// It is tempting to think that this step can be done BEFORE acquiring the commit timestamp, which would subsequently allow the client to silently abort
	// (by avoid flipping the bit on TS server if the transaction has to abort). However, this will result in a potential unlimited loop, where the client reads the same snapshot over and over
	// again, and each time aborts because the snapshot is no longer valid. However, the sweeper cannot make progress because this client aborts silently.
	// TODO: investigate if the above note is still valid for the new timestamp
	bool abortFlag = false;
	if (! isRecordAccessible(customerV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] (client " << clientID_ << ") Customer " << (int)customerV->customer.C_ID
				<< " (" << customerV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	else{
		for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
			if (! isRecordAccessible(items[olNumber]->writeTimestamp)){
				abortFlag = true;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] (client " << clientID_ << ") item " << (int)items[olNumber]->item.I_ID
						<< " (" << items[olNumber]->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
			}
			else if (! isRecordAccessible(stocks[olNumber]->writeTimestamp)){
				abortFlag = true;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] (client " << clientID_ << ") stock " << (int)stocks[olNumber]->stock.S_I_ID
						<< " (" << stocks[olNumber]->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
			}
		}
	}

	if (abortFlag == true) {
		DEBUG_COUT (CLASS_NAME, __func__, "(Client " << clientID_ << ") NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;
	}
	else DEBUG_COUT (CLASS_NAME, __func__, "[Info] (Client " << clientID_ << ") All received versions are consistent with READ snapshot, and all are unlocked");


	// ************************************************
	//	Acquire Commit timestamp
	// ************************************************
	primitive::timestamp_t cts = getNewCommitTimestamp();
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " acquired commit timestamp " << cts);


	// ************************************************
	// Acquire locks for records in write-set
	// ************************************************
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		primitive::lock_status_t 	lockStatus = 1;
		primitive::version_offset_t	versionOffset = stocks.at(olNumber)->writeTimestamp.getVersionOffset();
		primitive::client_id_t		clientID = clientID_;
		Timestamp lockTS (lockStatus, versionOffset, clientID, cts);

		lockStock(olNumber, cart.items.at(olNumber).I_ID, cart.items.at(olNumber).OL_SUPPLY_W_ID, stocks.at(olNumber)->writeTimestamp, lockTS);
	}

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	}

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		// Byte swapping, since values read by atomic operations are in Big Endian order
		//Timestamp& ts = localMemory_->getLockRegion()->getRegion()[olNumber];
		//Timestamp correctlyOrderedTS(ts.toUUL());
		//ts.copy(correctlyOrderedTS);
		localMemory_->getLockRegion()->getRegion()[olNumber] = utils::bigEndianToHost(localMemory_->getLockRegion()->getRegion()[olNumber]);
	}


	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " received the results for all the locks");

	// checks if locks are successful
	std::vector<uint8_t> unsuccesfulOrderLines;
	std::vector<uint8_t> succesfulOrderLines;

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		Timestamp existingLock(localMemory_->getLockRegion()->getRegion()[olNumber]);
		Timestamp &expectedLock = stocks.at(olNumber)->writeTimestamp;
		if (! existingLock.isEqual(expectedLock)){
			unsuccesfulOrderLines.push_back(olNumber);
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << "'s attemp to lock item " << (int)cart.items.at(olNumber).I_ID << " was NOT successful "
					<< "(expected: " << expectedLock << ", existing: " << existingLock << ")");
		}
		else {
			succesfulOrderLines.push_back(olNumber);
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << "'s attemp to lock item " << (int)cart.items.at(olNumber).I_ID << " was successful "
					<< "(expected = existing = " << existingLock << ")");
		}
	}

	if (unsuccesfulOrderLines.size() != 0){
		// some locks couldn't get acquired. must first release the successful ones, then abort
		for (auto const& olNumber: succesfulOrderLines){
			revertStockLock(olNumber, cart.items.at(olNumber).I_ID, cart.items.at(olNumber).OL_SUPPLY_W_ID);
		}
		for (auto const& olNumber: succesfulOrderLines){
			TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] (Client " << clientID_ << ") Lock reverted on stock " << stocks[olNumber]->stock);
			(void)olNumber;	// to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false

		}
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] (Client " << clientID_ << ") Lock on all items could not be acquired --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
		return trxResult;
	}

	// ************************************************
	//	Append old version to the versions list, and update the pointers list
	// ************************************************
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		updateStockPointers(olNumber, stocks[olNumber], cart.items.at(olNumber).OL_SUPPLY_W_ID);
		updateStockOlderVersions(olNumber, stocks[olNumber], cart.items.at(olNumber).OL_SUPPLY_W_ID);
	}
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " added the pointers and pointers");



	// ************************************************
	// Insert and unlock new records in write-set
	// ************************************************
	// insert a new row into New-Order and Order table.
	// O_CARRIER_ID is set to a null value. If the order includes only home order-lines, then O_ALL_LOCAL is set to 1, otherwise O_ALL_LOCAL is set to 0.
	primitive::lock_status_t 	lockStatus = 0;
	primitive::version_offset_t	versionOffset = 0;
	primitive::client_id_t		clientID = clientID_;
	Timestamp unlockTS(lockStatus, versionOffset, clientID, cts);

	// In District table, increment the next available order number for the district D_NEXT_O_ID
	uint32_t oID = retrieveAndIncrementDistrictNextOID(cart.wID, cart.dID);

	TPCC::OrderVersion* orderV = insertIntoOrder(oID, cart, timer, unlockTS);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted Order " << orderV->order);
	(void)orderV;	// to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false


	TPCC::NewOrderVersion* newOrderV = insertIntoNewOrder(oID, cart.wID, cart.dID, unlockTS);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted NewOrder " << newOrderV->newOrder);
	(void)newOrderV;	// to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false


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
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		bool remote = cart.items.at(olNumber).OL_SUPPLY_W_ID == 0;	// TODO

		lockStatus = 0;
		versionOffset = (primitive::version_offset_t)(stocks[olNumber]->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
		clientID = clientID_;
		Timestamp stockUnlockTS(lockStatus, versionOffset, clientID, cts);

		stocks[olNumber]->writeTimestamp.copy(stockUnlockTS);
		if (stocks[olNumber]->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY >= 10)
			stocks[olNumber]->stock.S_QUANTITY = (uint16_t)(stocks[olNumber]->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY);
		else
			stocks[olNumber]->stock.S_QUANTITY = (uint16_t)(stocks[olNumber]->stock.S_QUANTITY - cart.items.at(olNumber).OL_QUANTITY + 91);
		stocks[olNumber]->stock.S_YTD = (uint32_t)(stocks[olNumber]->stock.S_YTD + cart.items.at(olNumber).OL_QUANTITY);
		stocks[olNumber]->stock.S_ORDER_CNT = (uint16_t)(stocks[olNumber]->stock.S_ORDER_CNT + 1);
		if (remote)
			stocks[olNumber]->stock.S_REMOTE_CNT++;

		updateStock(olNumber, stocks[olNumber], cart.items.at(olNumber).OL_SUPPLY_W_ID);
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " updated stock " << stocks[olNumber]->stock << " (" << stockUnlockTS << ")"  << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);

		if (olNumber == cart.items.size() - 1)
			signaled = true;
		TPCC::OrderLineVersion *orderLineV = insertIntoOrderLine(olNumber, oID, cart, cart.items.at(olNumber), items[olNumber], stocks[olNumber], unlockTS, signaled);
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted OrderLine " << orderLineV->orderLine);
		(void)orderLineV;	// to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << "  successfully installed and unlocked all records");


	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	trxResult.result = TransactionResult::Result::COMMITTED;
	trxResult.reason = TransactionResult::Reason::SUCCESS;
	trxResult.cts = cts;
	submitResult(trxResult);
	DEBUG_COUT (CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << " sent trx result for CTS " << cts << " to the Oracle");

	return trxResult;
}

TPCC::ServerContext* TPCC::TPCCClient::getServerContext(uint16_t wID){
	size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	return dsCtx_[serverNum];
}

uint16_t TPCC::TPCCClient::getWarehouseOffsetOnServer(uint16_t wID){
	size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	return (uint16_t)(wID - (serverNum * config::tpcc_settings::WAREHOUSE_PER_SERVER));
}


void TPCC::TPCCClient::getReadTimestamp() {
	primitive::timestamp_t *lookupAddress = (primitive::timestamp_t*)oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector.rdmaHandler_.addr;
	uint32_t size = (uint32_t)(oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector.regionSize_ * sizeof(primitive::timestamp_t));

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_READ,
			oracleContext_->getQP(),
			localTimestampVector_->getRDMAHandler(),
			(uintptr_t)localTimestampVector_->getRegion(),
			&oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	//TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
}

void TPCC::TPCCClient::submitResult(TPCC::TransactionResult trxResult){
	localTimestampVector_->getRegion()[clientID_] = trxResult.cts;
	size_t tableOffset = (size_t)(clientID_ * sizeof(primitive::timestamp_t));		// offset of client's cts in timestampVector

	// The remote address of the timestamp
	primitive::timestamp_t *writeAddress = (primitive::timestamp_t*)(tableOffset + (uint64_t)oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(primitive::timestamp_t);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_WRITE,
			oracleContext_->getQP(),
			localTimestampVector_->getRDMAHandler(),
			(uintptr_t)&localTimestampVector_->getRegion()[clientID_],
			&oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
}

primitive::timestamp_t TPCC::TPCCClient::getNewCommitTimestamp() {
	primitive::timestamp_t output;
	output = (primitive::timestamp_t)(sessionState_->getNextEpoch() * clientCnt_ + clientID_);
	sessionState_->advanceEpoch();
	return output;
}

void TPCC::TPCCClient::retrieveWarehouseTax(uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();


	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	size_t payloadOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfWarehouse();		// offset of Warehouse in WarehouseVersion
	size_t fieldOffset = TPCC::Warehouse::getOffsetOfTax();								// offset of W_TAX in Warehouse

	// The remote address to read the warehouse tax
	float *lookupAddress =  (float*)(
			tableOffset
			+ payloadOffset
			+ fieldOffset
			+ ((uint64_t)remoteMemoryKeys->warehouseTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// Warehouse::W_TAX is of type float

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			localMemory_->getWarehouseHead()->getRDMAHandler(),
			(uintptr_t)&(localMemory_->getWarehouseHead()->getRegion()->warehouse.W_TAX),
			&remoteMemoryKeys->warehouseTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	// return localMemory_->getWarehouseHead()->getRegion()->warehouse.W_TAX;
}

void TPCC::TPCCClient::retrieveDistrictTax(uint16_t wID, uint8_t dID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();


	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfTax();		// offset of D_TAX in District

	// The remote address to read the district tax
	float *lookupAddress =  (float *)(
			tableOffset
			+ districtOffset
			+ fieldOffset
			+ ((uint64_t)remoteMemoryKeys->districtTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// District::D_TAX is of type float

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			localMemory_->getDistrictHead()->getRDMAHandler(),
			(uintptr_t)&(localMemory_->getDistrictHead()->getRegion()->district.D_TAX),
			&remoteMemoryKeys->districtTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	// return localMemory_->getDistrictHead()->getRegion()->district.D_TAX;
}

uint32_t TPCC::TPCCClient::retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfNextOID();		// offset of D_NEXT_O_ID in District

	// The remote address to read the district tax
	uint64_t *lookupAddress =  (uint64_t *)(
			tableOffset
			+ districtOffset
			+ fieldOffset
			+ ((uint64_t)remoteMemoryKeys->districtTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	size_t size = sizeof(localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID);

	TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(
			serverContext->getQP(),
			localMemory_->getDistrictHead()->getRDMAHandler(),
			(uintptr_t)&(localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID),
			&remoteMemoryKeys->districtTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			(uint64_t)1ULL,
			(uint32_t)size));
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// Byte swapping, since values read by atomic operations are in Big Endian order
	return (uint32_t) utils::bigEndianToHost(localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID);
}

TPCC::CustomerVersion* TPCC::TPCCClient::getCustomerInformation(uint16_t wID, uint8_t dID, uint32_t cID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* sizeof(TPCC::CustomerVersion));										// offset of CustomerVersion in CustomerTable

	// The remote address to read the customer info
	TPCC::CustomerVersion *lookupAddress =  (TPCC::CustomerVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->customerTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			localMemory_->getCustomerHead()->getRDMAHandler(),
			(uintptr_t)localMemory_->getCustomerHead()->getRegion(),
			&remoteMemoryKeys->customerTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	//TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	return localMemory_->getCustomerHead()->getRegion();
}


TPCC::ItemVersion* TPCC::TPCCClient::retrieveItem(uint8_t olNumber, uint32_t iID, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	TPCC::ItemVersion &itemV = localMemory_->getItemHead()->getRegion()[olNumber];

	size_t tableOffset = (size_t)(iID * sizeof(TPCC::ItemVersion));		// offset of ItemVersion in ItemTable

	// The remote address to read the item info
	TPCC::ItemVersion *lookupAddress =  (TPCC::ItemVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->itemTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::Item);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			localMemory_->getItemHead()->getRDMAHandler(),
			(uintptr_t)&itemV,
			&remoteMemoryKeys->itemTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	//TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	return &itemV;
}

TPCC::StockVersion* TPCC::TPCCClient::retrieveStock(uint8_t olNumber, uint32_t iID, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	TPCC::StockVersion &stockV = localMemory_->getStockHead()->getRegion()[olNumber];

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable

	// The remote address to read the item info
	TPCC::StockVersion *lookupAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			serverContext->getQP(),
			localMemory_->getStockHead()->getRDMAHandler(),
			(uintptr_t)&stockV,
			&remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// results are ready. now filling in the output arguments
	return &stockV;
}

void TPCC::TPCCClient::retrieveStockPointerList(uint8_t olNumber, uint32_t iID, uint16_t wID, bool signaled){
	TPCC::ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	// The remote address to read the item info
	Timestamp *readAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMemoryKeys->stockTableTimestampList.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock


	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getStockTS()->getRDMAHandler(),
			(uintptr_t)&localMemory_->getStockTS()->getRegion()[offset],
			&remoteMemoryKeys->stockTableTimestampList.rdmaHandler_,
			(uintptr_t)readAddress,
			size,
			signaled));
}


void TPCC::TPCCClient::updateStockPointers(uint8_t olNumber, StockVersion *oldHead, uint16_t wID) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localMemory_->getStockTS()->getRegion();
	size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[offset + i + 1] = versionArray[offset + i];

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[offset + 0] = oldHead->writeTimestamp;

	TPCC::ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	// The remote address to read the item info
	Timestamp *writeAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMemoryKeys->stockTableTimestampList.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));


	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getStockTS()->getRDMAHandler(),
			(uintptr_t)&versionArray[offset],
			&remoteMemoryKeys->stockTableTimestampList.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
}

void TPCC::TPCCClient::updateStockOlderVersions(uint8_t olNumber, StockVersion *oldHead, uint16_t wID) {
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	StockVersion *localBuffer = &localMemory_->getStockOlderVersions()->getRegion()[olNumber];
	memcpy(localBuffer, oldHead, sizeof(TPCC::StockVersion));


	TPCC::ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM * sizeof(StockVersion));
	size_t circularBufferOffset = (size_t) versionOffset * sizeof(StockVersion);

	// The remote address to read the item info
	StockVersion *writeAddress =  (StockVersion *)(tableOffset + circularBufferOffset + (uint64_t)remoteMemoryKeys->stockTableOlderVersions.rdmaHandler_.addr);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(StockVersion);


	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getStockOlderVersions()->getRDMAHandler(),
			(uintptr_t)localBuffer,
			&remoteMemoryKeys->stockTableOlderVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
}

TPCC::OrderVersion* TPCC::TPCCClient::insertIntoOrder(uint32_t oID, TPCC::Cart &cart, time_t timer, Timestamp writeTimestamp){
	ServerContext* serverContext = getServerContext(cart.wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(cart.wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	TPCC::OrderVersion *ov = localMemory_->getOrderHead()->getRegion();

	ov->writeTimestamp.copy(writeTimestamp);

	ov->order.O_ID = oID;
	ov->order.O_W_ID = cart.wID;
	ov->order.O_D_ID = cart.dID;
	ov->order.O_C_ID = cart.cID;
	ov->order.O_ENTRY_D = timer;
	ov->order.O_CARRIER_ID = 0;
	ov->order.O_OL_CNT = (uint8_t)cart.items.size();
	ov->order.O_ALL_LOCAL = 1;	// TODO: must be fixed

	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + cart.dID)
			* config::tpcc_settings::ORDER_PER_DISTRICT + oID) * sizeof(TPCC::OrderVersion));	// offset of OrderVersion in OrderTable

	// The remote address to which the order will be written
	TPCC::OrderVersion *writeAddress =  (TPCC::OrderVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->orderTableHeadVersions.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getOrderHead()->getRDMAHandler(),
			(uintptr_t)ov,
			&remoteMemoryKeys->orderTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return ov;
}

TPCC::NewOrderVersion* TPCC::TPCCClient::insertIntoNewOrder(uint32_t oID, uint16_t wID, uint8_t dID, Timestamp writeTimestamp){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	TPCC::NewOrderVersion *nov = localMemory_->getNewOrderHead()->getRegion();

	nov->writeTimestamp.copy(writeTimestamp);

	nov->newOrder.NO_O_ID = oID;
	nov->newOrder.NO_W_ID = wID;
	nov->newOrder.NO_D_ID = dID;

	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::NEWORDER_PER_DISTRICT + oID) * sizeof(TPCC::NewOrderVersion));	// offset of NewOrderVersion in NewOrderTable

	// The remote address to which the order will be written
	TPCC::NewOrderVersion *writeAddress =  (TPCC::NewOrderVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->newOrderTableHeadVersions.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::NewOrderVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getNewOrderHead()->getRDMAHandler(),
			(uintptr_t)nov,
			&remoteMemoryKeys->newOrderTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return nov;
}


/***
 * If the retrieved value for S_QUANTITY exceeds OL_QUANTITY by 10 or more, then S_QUANTITY is decreased by OL_QUANTITY; otherwise
 * S_QUANTITY is updated to (S_QUANTITY - OL_QUANTITY)+91. S_YTD is increased by OL_QUANTITY and S_ORDER_CNT is incremented by 1.
 * If the order-line is remote, then S_REMOTE_CNT is incremented by 1.
 */
error::ErrorType TPCC::TPCCClient::updateStock(uint8_t olNumber, TPCC::StockVersion *stockV, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + stockV->stock.S_I_ID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable

	// The remote address to read the item info
	TPCC::StockVersion *writeAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getStockHead()->getRDMAHandler(),
			(uintptr_t)&localMemory_->getStockHead()->getRegion()[olNumber],
			&remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			false));

	// TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return error::ErrorType::SUCCESS;
}

// |-- The amount for the item in the order (OL_AMOUNT) is computed as: OL_QUANTITY * I_PRICE
// |
// |-- The strings in I_DATA and S_DATA are examined. If they both include the string "ORIGINAL",
// |   the brand-generic field for that item is set to "B", otherwise, the brand-generic field is set to "G".
// |
// |-- A new row is inserted into the ORDER-LINE table to reflect the item on the order. OL_DELIVERY_D is set to a null value, OL_NUMBER is set to a unique value within
// |   all the ORDER-LINE rows that have the same OL_O_ID value, and OL_DIST_INFO is set to the content of S_DIST_xx, where xx represents the district number (OL_D_ID)

TPCC::OrderLineVersion* TPCC::TPCCClient::insertIntoOrderLine(uint8_t olNumber, uint32_t oID, Cart &cart, NewOrderItem &newOrderItem, TPCC::ItemVersion *itemV, TPCC::StockVersion *stockV, Timestamp &ts, bool signaled){
	ServerContext* serverContext = getServerContext(cart.wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(cart.wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();

	TPCC::OrderLineVersion &olv = localMemory_->getOrderLineHead()->getRegion()[olNumber];

	olv.writeTimestamp.copy(ts);

	olv.orderLine.OL_O_ID 			= oID;
	olv.orderLine.OL_D_ID 			= cart.dID;
	olv.orderLine.OL_W_ID 			= cart.wID;
	olv.orderLine.OL_NUMBER 		= olNumber;
	olv.orderLine.OL_I_ID 			= itemV->item.I_ID;
	olv.orderLine.OL_SUPPLY_W_ID 	= newOrderItem.OL_SUPPLY_W_ID;
	olv.orderLine.OL_DELIVERY_D 	= (time_t)0;
	olv.orderLine.OL_QUANTITY 		=  newOrderItem.OL_QUANTITY;
	olv.orderLine.OL_AMOUNT 		= (float)newOrderItem.OL_QUANTITY * itemV->item.I_PRICE;
	std::memcpy(olv.orderLine.OL_DIST_INFO, stockV->stock.S_DIST[cart.dID], 25);

	size_t tableOffset = (size_t)(
			(
					(warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + cart.dID)
					* config::tpcc_settings::ORDER_PER_DISTRICT + oID
			)
			* tpcc_settings::ORDER_MAX_OL_CNT + olNumber
	)
	* sizeof(TPCC::OrderLineVersion);

	// The remote address to read the item info
	TPCC::OrderLineVersion *writeAddress =  (TPCC::OrderLineVersion *)(tableOffset + ((uint64_t)remoteMemoryKeys->orderLineTableHeadVersions.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderLineVersion);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getOrderLineHead()->getRDMAHandler(),
			(uintptr_t)&olv,
			&remoteMemoryKeys->orderLineTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			signaled));

	//TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	return &olv;
}

void TPCC::TPCCClient::lockStock(uint8_t olNumber, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();


	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion

	// The remote address of the timestamp
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(uint64_t);

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			serverContext->getQP(),
			localMemory_->getLockRegion()->getRDMAHandler(),
			(uintptr_t)&localMemory_->getLockRegion()->getRegion()[olNumber],
			&remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			oldTS.toUUL(),
			newTS.toUUL()));
}

void TPCC::TPCCClient::revertStockLock(uint8_t olNumber, uint32_t iID, uint16_t wID){
	ServerContext* serverContext = getServerContext(wID);
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	TPCC::ServerMemoryKeys* remoteMemoryKeys = serverContext->getRemoteMemoryKeys()->getRegion();


	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion

	// The remote address of the timestamp
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(Timestamp);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_WRITE,
			serverContext->getQP(),
			localMemory_->getStockHead()->getRDMAHandler(),
			(uintptr_t)&localMemory_->getStockHead()->getRegion()[olNumber].writeTimestamp,
			&remoteMemoryKeys->stockTableHeadVersions.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			true));
}

inline
std::string TPCC::TPCCClient::pointer_to_string(Timestamp* ts) const{
	std::ostringstream stream;
	for (size_t i = 0; i < config::tpcc_settings::VERSION_NUM; i++)
		stream << ts[i] << ", ";
	return stream.str();
}

TPCC::TPCCClient::~TPCCClient(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
	delete localTimestampVector_;
	delete oracleContext_;
	delete context_;
	delete sessionState_;
}
