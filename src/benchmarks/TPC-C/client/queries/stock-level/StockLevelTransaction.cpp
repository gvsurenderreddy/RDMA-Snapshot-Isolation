/*
 * StockLevelTransaction.cpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#include "../../../../../util/utils.hpp"
#include "StockLevelTransaction.hpp"

#define CLASS_NAME "StockLvlTrx"
namespace TPCC {

StockLevelTransaction::StockLevelTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: BaseTransaction(os, "StockLevel", executor, clientID, clientCnt, dsCtx, sessionState, random, context, oracleContext,localTimestampVector){
	localMemory_ 	= new StockLevelLocalMemory(os_, *context_);
}

StockLevelTransaction::~StockLevelTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

TPCC::StockLevelCart StockLevelTransaction::buildCart(){
	StockLevelCart cart;

	// 2.8.1.1 Each terminal must use a unique value of (W_ID, D_ID) that is constant over the whole measurement, i.e., D_IDs cannot be re-used within a warehouse.
	cart.wID = sessionState_->getHomeWarehouseID();
	cart.dID = sessionState_->getHomeDistrictID();

	// 2.8.1.2 The threshold of minimum quantity in stock (threshold ) is selected at random within [10 .. 20].
	cart.threshold = (unsigned) random_->number(10, 20);
	return cart;
}

TPCC::TransactionResult StockLevelTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;
	TPCC::DistrictVersion *districtV;



	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	StockLevelCart cart = buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << (int)clientID_ << ": Cart: " << cart);


	// ************************************************
	//	Acquire read timestamp
	// ************************************************
	executor_.getReadTimestamp(*localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP(), true);
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.getReadTimestamp()
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle");


	// ************************************************
	//	Read records in read-set
	// ************************************************
	TPCC::ServerContext *serverCtx = getServerContext(cart.wID);

	executor_.retrieveDistrict(
			cart.wID,
			cart.dID,
			*localMemory_->getDistrictHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			true);

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	districtV = localMemory_->getDistrictHead()->getRegion();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District: " << districtV->district << " with D_NEXT_OID = " << (int)districtV->district.D_NEXT_O_ID);
	if (! isRecordAccessible(districtV->writeTimestamp)){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << districtV->district
				<< " (" << districtV->writeTimestamp << ") is not consistent (locked or from a later snapshot) --> ** ABORT **");

		// TODO: IMPORTANT: Instead of aborting, the client should fetch the older versions of district
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;;
	}

	executor_.getDistinctItemsForLastTwentyOrders(
			clientID_,
			cart.wID,
			cart.dID,
			(uint32_t)districtV->district.D_NEXT_O_ID,
			*serverCtx->getIndexRequestMessage(),
			*serverCtx->getLast20OrdersIndexResponseMessage(),
			serverCtx->getQP(),
			true);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: ITEMS_FOR_LAST_20_ORDERS. Parameters: wID = " << (int)cart.wID
			<< ", dID = " << (int)cart.dID << ", d_next_oid = " << (uint32_t)districtV->district.D_NEXT_O_ID);

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.getDistinctItemsForLastTwentyOrders()

	TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));	// for executor_.getDistinctItemsForLastTwentyOrders()

	if (serverCtx->getLast20OrdersIndexResponseMessage()->getRegion()->isSuccessful == false){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. Unsuccessful --> ** Abort **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;;
	}

	size_t orderlinesCnt = serverCtx->getLast20OrdersIndexResponseMessage()->getRegion()->orderlinesCnt;
	uint32_t *itemIDs = serverCtx->getLast20OrdersIndexResponseMessage()->getRegion()->itemIDs;

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. # distinct items = " << orderlinesCnt);

	// if there is no order, no further work is needed. just commit.
	if (orderlinesCnt == 0) {
		trxResult.result = TransactionResult::Result::COMMITTED;
		return trxResult;
	}


	bool signaled = false;
	for (size_t i=0; i < orderlinesCnt; i++){
		if (i == orderlinesCnt - 1)
			signaled = true;
		executor_.retrieveStock(
				i,
				itemIDs[i],
				cart.wID,
				*localMemory_->getStockHead(),
				getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions,
				getServerContext(cart.wID)->getQP(),
				signaled);
	}

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.retrieveStock()


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	bool abortFlag = false;
	for (size_t i=0; i < orderlinesCnt; i++){
		if (! isRecordAccessible(localMemory_->getStockHead()->getRegion()[i].writeTimestamp)){
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Stock " << localMemory_->getStockHead()->getRegion()[i].stock
					<< " (" << localMemory_->getStockHead()->getRegion()[i].writeTimestamp << ") is not consistent (locked or from a later snapshot)");
			abortFlag = true;
			break;
		}
	}

	if (abortFlag){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "Client " << clientID_ << ": NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. # orderlines = " << orderlinesCnt);
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;
	}
	else DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": All received versions are consistent with READ snapshot, and all are unlocked");


	// ************************************************
	//	Perform computation
	// ************************************************
	// Count stocks whose quantity is lower than the given threshold
	unsigned lowStockCnt = 0;
	for (size_t i=0; i < orderlinesCnt; i++){
		if (localMemory_->getStockHead()->getRegion()[i].stock.S_QUANTITY < cart.threshold)
			lowStockCnt++;
	}
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": The number of stocks with low quantity: " << (int)lowStockCnt);

	trxResult.result = TransactionResult::Result::COMMITTED;
	return trxResult;
}

} /* namespace TPCC */
