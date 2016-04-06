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

int NewOrderTransaction::oops = 0;

NewOrderTransaction::NewOrderTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: BaseTransaction("New Order", clientID, clientCnt, dsCtx, sessionState, random, context, oracleContext,localTimestampVector){
	localMemory_ 	= new NewOrderLocalMemory(*context_);
}

NewOrderTransaction::~NewOrderTransaction() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}


NewOrderCart NewOrderTransaction::buildCart(){
	NewOrderCart cart;

	// 2.4.1.1 the home warehouse number (W_ID) is constant over the whole measurement interval
	// cart.wID = (uint16_t) random_.number(0, config::tpcc_settings::WAREHOUSE_CNT - 1);
	cart.wID = sessionState_->getHomeWarehouseID();

	// 2.4.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart.dID = (uint8_t) random_->number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.4.1.2 The non-uniform random customer number (C_ID) is selected using the NURand (1023,1,3000) function
	cart.cID = (uint32_t) random_->NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);

	// 2.4.1.3 The number of items in the order (ol_cnt) is randomly selected within [5 .. 15] (an average of 10)
	uint8_t ol_cnt = (uint8_t ) random_->number(tpcc_settings::ORDER_MIN_OL_CNT, tpcc_settings::ORDER_MAX_OL_CNT);

	// 2.4.1.4 A fixed 1% of the New-Order transactions are chosen at random to simulate user data entry errors and exercise the performance of rolling back update transactions.
	// bool rollback = random_.number(1, 100) == 1;

	std::vector<uint32_t> uniqueItemIDs(ol_cnt);
	std::set<uint32_t> uniqueIDSets = random_->selectUniqueNonUniformIds(8191, ol_cnt, 0, config::tpcc_settings::ITEMS_CNT - 1);
	std::copy(uniqueIDSets.begin(), uniqueIDSets.end(), uniqueItemIDs.begin());

	for (int i = 0; i < ol_cnt; ++i) {
		NewOrderItem noi;
		// A non-uniform random item number (OL_I_ID) is selected using the NURand (8191,1,100000) function
		//noi.I_ID = (uint32_t) random_.NURand(8191, 0, config::tpcc_settings::ITEMS_CNT - 1);
		noi.I_ID = uniqueItemIDs.at(i);

		// TPC-C suggests generating a number in range (1, 100) and selecting remote on 1 (clause 2.4.1.5.2)
		// This provides more variation, and lets us tune the fraction of "remote" transactions.
		bool remote = random_->number(0, 1) <= config::tpcc_settings::REMOTE_WAREHOUSE_PROB;
		if (config::tpcc_settings::WAREHOUSE_CNT > 1 && remote) {
			noi.OL_SUPPLY_W_ID = (uint16_t) random_->numberExcluding(0, config::tpcc_settings::WAREHOUSE_CNT - 1, sessionState_->getHomeWarehouseID());
		} else {
			noi.OL_SUPPLY_W_ID = sessionState_->getHomeWarehouseID();
		}

		// 2.4.1.5.3 A quantity (OL_QUANTITY) is randomly selected within [1 .. 10]
		noi.OL_QUANTITY = (uint8_t) random_->number(1, 10);
		cart.items.push_back(noi);
	}
	return cart;
}


TPCC::TransactionResult NewOrderTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;

	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	NewOrderCart cart = buildCart();
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart);


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
	TPCC::WarehouseVersion *warehouseV = localMemory_->getWarehouseHead()->getRegion();


	// From District table, retrieve the row with matching D_W_ID and D_ID.
	executor_.retrieveDistrict(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	TPCC::DistrictVersion *districtV = localMemory_->getDistrictHead()->getRegion();


	// From Customer table, retrieve C_DISCOUNT (the customer's discount rate), C_LAST (the customer's last name), and C_CREDIT (the customer's credit status)
	executor_.retrieveCustomer(
			cart.wID,
			cart.dID,
			cart.cID,
			*localMemory_->getCustomerHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
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
				getServerContext(cart.wID)->getQP(),
				false);
		items.push_back(&localMemory_->getItemHead()->getRegion()[olNumber]);

		executor_.retrieveStock(
				olNumber,
				cart.items.at(olNumber).I_ID,
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockHead(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
				false);
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
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Warehouse: " << warehouseV->warehouse);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District: " << districtV->district);
	DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Customer " << customerV->customer);
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Item: " << items[olNumber]->item);
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved stock: " << *stocks[olNumber] << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);
		size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock
		DEBUG_COUT (CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved the pointer list for Stock " << stocks[olNumber]->stock
				<< ": " << pointer_to_string(&localMemory_->getStockTS()->getRegion()[offset]) << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID );

		(void) offset; // to avoid getting the "unused variable" warning when compiled with DEBUG_ENABLED = false
	}

	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	bool abortFlag = false;
	bool realAbort = false;
	if (! isRecordAccessible(warehouseV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Warehouse " << warehouseV->warehouse
				<< " (" << warehouseV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(districtV->writeTimestamp)){
		realAbort  = true;
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << districtV->district
				<< " (" << districtV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	if (! isRecordAccessible(customerV->writeTimestamp)){
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Customer " << (int)customerV->customer.C_ID
				<< " (" << customerV->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
	}
	else{
		for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
			if (! isRecordAccessible(items[olNumber]->writeTimestamp)){
				abortFlag = true;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": item " << (int)items[olNumber]->item.I_ID
						<< " (" << items[olNumber]->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
			}
			else if (! isRecordAccessible(stocks[olNumber]->writeTimestamp)){
				realAbort = true;
				abortFlag = true;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": stock " << (int)stocks[olNumber]->stock.S_I_ID
						<< " (" << stocks[olNumber]->writeTimestamp << ") is not consistent (locked or from a later snapshot)");
			}
		}
	}

	if (realAbort == false && abortFlag == true) {
		oops++;
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

	// lock district
	primitive::lock_status_t 	lockStatus 		= 1;
	primitive::version_offset_t	versionOffset	= districtV->writeTimestamp.getVersionOffset();
	Timestamp districtLockTS (lockStatus, versionOffset, clientID_, cts);

	executor_.lockDistrict(
			*districtV,
			districtLockTS,
			*localMemory_->getDistrictLockRegion(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP());

	// lock stocks
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
				*localMemory_->getStocksLocksRegion(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP());
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for district
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++)
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for each stock

	// Byte swapping, since values read by atomic operations are in Big Endian order
	*localMemory_->getDistrictLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getDistrictLockRegion()->getRegion());
	Timestamp districtExistingLock(*localMemory_->getDistrictLockRegion()->getRegion());
	Timestamp &districtExpectedLock = districtV->writeTimestamp;

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++)
		localMemory_->getStocksLocksRegion()->getRegion()[olNumber] = utils::bigEndianToHost(localMemory_->getStocksLocksRegion()->getRegion()[olNumber]);

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": received the results for all the locks");

	// checks if locks are successful
	abortFlag = false;

	if (!districtExistingLock.isEqual(districtExpectedLock)) {
		abortFlag = true;
		DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock district " << districtV->district << " was NOT successful "
				<< "(expected: " << districtExpectedLock << ", existing: " << districtExistingLock << ")");
	}

	std::vector<uint8_t> unsuccesfulOrderLines;
	std::vector<uint8_t> succesfulOrderLines;
	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		Timestamp existingLock(localMemory_->getStocksLocksRegion()->getRegion()[olNumber]);
		Timestamp &expectedLock = stocks.at(olNumber)->writeTimestamp;
		if (! existingLock.isEqual(expectedLock)){
			unsuccesfulOrderLines.push_back(olNumber);
			abortFlag = true;
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock item " << (int)cart.items.at(olNumber).I_ID << " was NOT successful "
					<< "(expected: " << expectedLock << ", existing: " << existingLock << ")");
		}
		else {
			succesfulOrderLines.push_back(olNumber);
			DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock item " << (int)cart.items.at(olNumber).I_ID << " was successful "
					<< "(expected = existing = " << existingLock << ")");
		}
	}

	unsigned successfulLocksCnt = 0;

	if (abortFlag){
		// some locks couldn't get acquired. must first release the successful ones, then abort
		if (districtExistingLock.isEqual(districtExpectedLock)){
			successfulLocksCnt++;
			executor_.revertDistrictLock(
					*localMemory_->getDistrictHead(),
					getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
					getServerContext(cart.wID)->getQP(),
					true);
		}

		for (auto const& olNumber: succesfulOrderLines){
			successfulLocksCnt++;
			executor_.revertStockLock(
					olNumber,
					cart.items.at(olNumber).I_ID,
					cart.items.at(olNumber).OL_SUPPLY_W_ID,
					*localMemory_->getStockHead(),
					getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
					getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
					true);
		}
		for (unsigned i = 0; i < successfulLocksCnt; i++){
			TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
			DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted lock");

		}
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": could not acquire lock on all items --> ** ABORT **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
		return trxResult;
	}

	// ************************************************
	//	Append old version to the versions list, and update the pointers list
	// ************************************************
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

	for (uint8_t olNumber = 0; olNumber < cart.items.size(); olNumber++){
		executor_.updateStockPointers(olNumber, stocks[olNumber], cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockTS(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableTimestampList,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
				false);

		executor_.updateStockOlderVersions(
				olNumber,
				stocks[olNumber],
				cart.items.at(olNumber).OL_SUPPLY_W_ID,
				*localMemory_->getStockOlderVersions(),
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getRemoteMemoryKeys()->getRegion()->stockTableOlderVersions,
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
				false);
	}
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": added the pointers and older version");



	// ************************************************
	// Insert and unlock new records in write-set
	// ************************************************
	// update district (increment the next available order number D_NEXT_O_ID)
	lockStatus = 0;
	versionOffset = (primitive::version_offset_t)(districtV->writeTimestamp.getVersionOffset() + 1) % config::tpcc_settings::VERSION_NUM;
	districtV->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	uint32_t oID = (uint32_t)districtV->district.D_NEXT_O_ID;
	districtV->district.D_NEXT_O_ID = (uint32_t)districtV->district.D_NEXT_O_ID + 1;
	executor_.updateDistrict(
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated district " << districtV->district);

	// insert a new row into New-Order and Order table.
	// O_CARRIER_ID is set to a null value. If the order includes only home order-lines, then O_ALL_LOCAL is set to 1, otherwise O_ALL_LOCAL is set to 0.
	TPCC::OrderVersion *ov = localMemory_->getOrderHead()->getRegion();

	versionOffset = 0;
	ov->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
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
			getServerContext(cart.wID)->getQP(),
			false);

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted Order with oID: " << nextOrderID_ << ", wID: " << cart.wID << ", dID: " << cart.dID);


	TPCC::NewOrderVersion *nov = localMemory_->getNewOrderHead()->getRegion();
	versionOffset = 0;
	ov->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
	nov->newOrder.NO_O_ID = oID;
	nov->newOrder.NO_W_ID = cart.wID;
	nov->newOrder.NO_D_ID = cart.dID;

	executor_.insertIntoNewOrder(
			clientID_,
			nextNewOrderID_,
			cart.wID,
			*localMemory_->getNewOrderHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			false);
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted NewOrder with noID: " << nextNewOrderID_ << ", wID: " << cart.wID << ", dID: " << cart.dID);

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
		Timestamp stockUnlockTS(lockStatus, versionOffset, clientID_, cts);

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
				getServerContext(cart.items.at(olNumber).OL_SUPPLY_W_ID)->getQP(),
				false);
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated stock " << stocks[olNumber]->stock << " (" << stockUnlockTS << ")"  << " from warehouse " << (int)cart.items.at(olNumber).OL_SUPPLY_W_ID);

		if (olNumber == cart.items.size() - 1)
			signaled = true;

		TPCC::OrderLineVersion &olv = localMemory_->getOrderLineHead()->getRegion()[olNumber];
		versionOffset = 0;
		ov->writeTimestamp.setAll(lockStatus, versionOffset, clientID_, cts);
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

		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": inserted OrderLine with oID: " << oID << ", wID: " << cart.wID << ", dID: " << cart.dID );
		nextOrderLineID_++;
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully installed and unlocked all records");

	// update the index
	TPCC::ServerContext *serverCtx = getServerContext(cart.wID);
	executor_.registerOrder(
			clientID_,
			cart.wID,
			cart.dID,
			cart.cID,
			oID,
			nextOrderID_,
			nextNewOrderID_,
			nextOrderLineID_,
			(uint8_t)cart.items.size(),
			*serverCtx->getIndexRequestMessage(),
			*serverCtx->getIndexResponseMessage(),
			serverCtx->getQP(),
			false);
	DEBUG_COUT(CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: REGISTER_INDEX. Parameters: wID = " << (int)cart.wID
			<< ", dID = " << (int)cart.dID << ", cID = " << (int)cart.cID << ", oID = " << (int)oID << ", regionOffset: " << (int)nextOrderID_ << ", #orderlines: " << cart.items.size());

	// receive the acknowledgement of the index update request
	TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));
	assert(serverCtx->getIndexResponseMessage()->getRegion()->isSuccessful == true);
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received for message . Type = REGISTER_ORDER");


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

	nextOrderID_++;
	nextNewOrderID_++;

	return trxResult;
}


} /* namespace TPCC */
