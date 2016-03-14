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
#include <cassert>

#define CLASS_NAME "TPCCClient"

TPCC::TPCCClient::TPCCClient(unsigned instanceNum, uint16_t homeWarehouseID, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort),
  nextOrderID_(0),
  nextNewOrderID_(0),
  nextOrderLineID_(0){
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


	double abort_rate = 1 - (double)committedCnt / config::tpcc_settings::NEWORDER_TRANSACTION_CNT;
	double inconsistentSnapshotRatio = (abortCnt==0) ? 0 : (double)abortDueToInconsistentSnapshot/abortCnt;
	double unsuccessfulLockRatio = (abortCnt==0) ? 0 : (double)abortDueToUnsuccessfulLock/abortCnt;
	double trxsPerSec = (double)(committedCnt / (double)(microElapsedTime / (1000 * 1000) ));


	std::cout << "[Stat] " << committedCnt << " committed, " << abortCnt << " aborted. abort rate:	" << abort_rate << std::endl;
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
	executor_.getReadTimestamp(*localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP());
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " received read snapshot from oracle");


	// ************************************************
	//	Read records in read-set
	// ************************************************
	// From Warehouse table, retrieve W_TAX
	executor_.retrieveWarehouseTax(
			cart.wID,
			*localMemory_->getWarehouseHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	// From District table, retrieve D_TAX
	executor_.retrieveDistrictTax(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	// From Customer table, retrieve C_DISCOUNT (the customer's discount rate), C_LAST (the customer's last name), and C_CREDIT (the customer's credit status)
	executor_.getCustomerInformation(
			cart.wID,
			cart.dID,
			cart.cID,
			*localMemory_->getCustomerHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	TPCC::CustomerVersion *customerV = localMemory_->getCustomerHead()->getRegion();

	std::vector<TPCC::ItemVersion*> items;
	std::vector<TPCC::StockVersion*> stocks;

	// Retrieve item and stock
	bool signaled = false;
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		executor_.retrieveItem(
				olNumber,
				cart.items.at(olNumber).I_ID,
				cart.wID,
				*localMemory_->getItemHead(),
				getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->itemTableHeadVersions,
				getServerContext(cart.wID)->getQP());
		items.push_back(&localMemory_->getItemHead()->getRegion()[olNumber]);

		executor_.retrieveStock(
				olNumber,
				cart.items.at(olNumber).I_ID,
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockHead(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
		stocks.push_back(&localMemory_->getStockHead()->getRegion()[olNumber]);

		// make the last request a signaled one
		if (olNumber == cart.items.size() - 1)
			signaled = true;

		executor_.retrieveStockPointerList(
				olNumber,
				cart.items.at(olNumber).I_ID,
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockTS(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableTimestampList,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
				signaled);

	}

	// wait for all outstanding RDMA requests to finish
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// printing for debugging purposes
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Warehouse " << (int)cart.wID);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved District D_W_ID: " << (int)cart.wID << ", D_ID: " << (int)cart.dID);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Customer " << customerV->customer);
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved Item: " << items[olNumber]->item);
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved stock: " << *stocks[olNumber] << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);

		size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << " retrieved the pointer list for Stock " << (int)stocks[olNumber]->stock.S_I_ID
				<< ": " << pointer_to_string(&localMemory_->getStockTS()->getRegion()[offset]) << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID );

		(void) offset; // to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false
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

		executor_.lockStock(
				olNumber,
				cart.items.at(olNumber).I_ID,
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				stocks.at(olNumber)->writeTimestamp,
				lockTS,
				*localMemory_->getLockRegion(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
	}

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++)
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++)
		localMemory_->getLockRegion()->getRegion()[olNumber] = utils::bigEndianToHost(localMemory_->getLockRegion()->getRegion()[olNumber]);


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
			executor_.revertStockLock(
					olNumber,
					cart.items.at(olNumber).I_ID,
					cart.items.at(olNumber).OL_SUPPLY_W_ID,
					*localMemory_->getStockHead(),
					getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
					getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
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
		executor_.updateStockPointers(olNumber, stocks[olNumber], cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockTS(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableTimestampList,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());

		executor_.updateStockOlderVersions(
				olNumber,
				stocks[olNumber],
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockOlderVersions(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableOlderVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
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
	executor_.retrieveAndIncrementDistrictNextOID(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	// Byte swapping, since values read by atomic operations are in Big Endian order
	uint32_t oID = utils::bigEndianToHost(localMemory_->getDistrictHead()->getRegion()->district.D_NEXT_O_ID);


	TPCC::OrderVersion *ov = localMemory_->getOrderHead()->getRegion();
	ov->writeTimestamp.copy(unlockTS);
	ov->order.O_ID = oID;
	ov->order.O_W_ID = cart.wID;
	ov->order.O_D_ID = cart.dID;
	ov->order.O_C_ID = cart.cID;
	ov->order.O_ENTRY_D = timer;
	ov->order.O_CARRIER_ID = 0;
	ov->order.O_OL_CNT = (uint8_t)cart.items.size();
	ov->order.O_ALL_LOCAL = 1;	// TODO: must be fixed

	executor_.insertIntoOrder(
			clientID_,
			nextOrderID_,
			cart.wID,
			*localMemory_->getOrderHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted Order with oID: " << nextOrderID_ << ", wID: " << cart.wID << ", dID: " << cart.dID);
	nextOrderID_++;


	TPCC::NewOrderVersion *nov = localMemory_->getNewOrderHead()->getRegion();
	nov->writeTimestamp.copy(unlockTS);
	nov->newOrder.NO_O_ID = oID;
	nov->newOrder.NO_W_ID = cart.wID;
	nov->newOrder.NO_D_ID = cart.dID;

	executor_.insertIntoNewOrder(
			clientID_,
			nextNewOrderID_,
			cart.wID,
			*localMemory_->getNewOrderHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
			getServerContext(cart.wID)->getQP());
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted NewOrder with noID: " << nextNewOrderID_ << ", wID: " << cart.wID << ", dID: " << cart.dID);
	nextNewOrderID_++;

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

		executor_.updateStock(
				olNumber,
				stocks[olNumber],
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockHead(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " updated stock " << stocks[olNumber]->stock << " (" << stockUnlockTS << ")"  << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);

		if (olNumber == cart.items.size() - 1)
			signaled = true;

		TPCC::OrderLineVersion &olv = localMemory_->getOrderLineHead()->getRegion()[olNumber];
		olv.writeTimestamp.copy(unlockTS);
		olv.orderLine.OL_O_ID 			= oID;
		olv.orderLine.OL_D_ID 			= cart.dID;
		olv.orderLine.OL_W_ID 			= cart.wID;
		olv.orderLine.OL_NUMBER 		= olNumber;
		olv.orderLine.OL_I_ID 			= items[olNumber]->item.I_ID;
		olv.orderLine.OL_SUPPLY_W_ID 	= cart.items.at(olNumber).OL_SUPPLY_W_ID;
		olv.orderLine.OL_DELIVERY_D 	= (time_t)0;
		olv.orderLine.OL_QUANTITY 		=  cart.items.at(olNumber).OL_QUANTITY;
		olv.orderLine.OL_AMOUNT 		= (float)cart.items.at(olNumber).OL_QUANTITY * items[olNumber]->item.I_PRICE;
		std::memcpy(olv.orderLine.OL_DIST_INFO, stocks[olNumber]->stock.S_DIST[cart.dID], 25);

		executor_.insertIntoOrderLine(
				clientID_,
				nextOrderLineID_,
				olNumber,
				cart.wID,
				*localMemory_->getOrderLineHead(),
				getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
				getServerContext(cart.wID)->getQP(),
				signaled);

		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " inserted OrderLine with oID: " << oID << ", wID: " << cart.wID << ", dID: " << cart.dID );
		nextOrderLineID_++;
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << " successfully installed and unlocked all records");


	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	trxResult.result = TransactionResult::Result::COMMITTED;
	trxResult.reason = TransactionResult::Reason::SUCCESS;
	trxResult.cts = cts;
	localTimestampVector_->getRegion()[clientID_] = trxResult.cts;
	executor_.submitResult(clientID_, *localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP());
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << " sent trx result for CTS " << cts << " to the Oracle");

	return trxResult;
}

TPCC::ServerContext* TPCC::TPCCClient::getServerContext(uint16_t wID){
	size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	return dsCtx_[serverNum];
}

primitive::timestamp_t TPCC::TPCCClient::getNewCommitTimestamp() {
	primitive::timestamp_t output;
	output = (primitive::timestamp_t)(sessionState_->getNextEpoch() * clientCnt_ + clientID_);
	sessionState_->advanceEpoch();
	return output;
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
