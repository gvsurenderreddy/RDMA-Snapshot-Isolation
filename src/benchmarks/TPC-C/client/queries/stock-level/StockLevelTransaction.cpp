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

StockLevelTransaction::StockLevelTransaction(TPCCClient &client, DBExecutor &executor)
: BaseTransaction("StockLevel", client, executor){
	localMemory_ 	= new StockLevelLocalMemory(os_, context_);
}

StockLevelTransaction::~StockLevelTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

void StockLevelTransaction::buildCart(){
	cart_.reset();

	// 2.8.1.1 Each terminal must use a unique value of (W_ID, D_ID) that is constant over the whole measurement, i.e., D_IDs cannot be re-used within a warehouse.
	cart_.wID = sessionState_.getHomeWarehouseID();
	cart_.dID = sessionState_.getHomeDistrictID();

	// 2.8.1.2 The threshold of minimum quantity in stock (threshold ) is selected at random within [10 .. 20].
	cart_.threshold = (unsigned) random_.number(10, 20);
}

void StockLevelTransaction::initilizeTransaction(){
	// ************************************************
	//	Constructing the transaction cart
	// ************************************************
	buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart_);
}

TPCC::TransactionResult StockLevelTransaction::doOne(){
	TransactionResult trxResult;
	TPCC::DistrictVersion *districtV;


	// ************************************************
	//	Acquire read timestamp
	// ************************************************
	executor_.getReadTimestamp(localTimestampVector_, oracleContext_.getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_.getQP(), true);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle");


	// ************************************************
	//	Read records in read-set
	// ************************************************
	size_t serverNum = Warehouse::getServerNum(cart_.wID);

	executor_.retrieveDistrict(cart_.wID, cart_.dID, *localMemory_->getDistrictHead(), false);
	executor_.retrieveDistrictPointerList(cart_.wID, cart_.dID, *localMemory_->getDistrictTS(), true);

	executor_.synchronizeSendEvents();

	districtV = localMemory_->getDistrictHead()->getRegion();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved District: " << districtV->district << " with D_NEXT_OID = " << (int)districtV->district.D_NEXT_O_ID);

	if (! isRecordAccessible(districtV->writeTimestamp)){
		int ind = findValidVersion(localMemory_->getDistrictTS()->getRegion(), config::tpcc_settings::VERSION_NUM);
		if (ind < 0) {
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << *districtV << " and none of its versions are not consistent (locked or from a later snapshot) --> ** ABORT **");
			trxResult.result = TransactionResult::Result::ABORTED;
			trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
			return trxResult;;
		}
		else{
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": District " << *districtV << " is not consistent, but its " << ind << "'s version is consistent (" << localMemory_->getDistrictTS()->getRegion()[ind] << ")");
			executor_.retrieveDistrictOlderVersion(cart_.wID, cart_.dID, (size_t)ind, *localMemory_->getDistrictHead(), true);
			executor_.synchronizeSendEvents();
			// note that districtV is not updated with a consistent version
		}
	}

	executor_.getDistinctItemsForLastTwentyOrders(
			clientID_,
			cart_.wID,
			cart_.dID,
			(uint32_t)districtV->district.D_NEXT_O_ID,
			*dsCtx_[serverNum]->getIndexRequestMessage(),
			*dsCtx_[serverNum]->getLast20OrdersIndexResponseMessage(),
			dsCtx_[serverNum]->getQP(),
			true);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: ITEMS_FOR_LAST_20_ORDERS. Parameters: wID = " << (int)cart_.wID
			<< ", dID = " << (int)cart_.dID << ", d_next_oid = " << (uint32_t)districtV->district.D_NEXT_O_ID);

	executor_.synchronizeNetworkEvents();

	if (dsCtx_[serverNum]->getLast20OrdersIndexResponseMessage()->getRegion()->isSuccessful == false){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. Unsuccessful --> ** Abort **");
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
		return trxResult;;
	}

	size_t orderlinesCnt = dsCtx_[serverNum]->getLast20OrdersIndexResponseMessage()->getRegion()->orderlinesCnt;
	uint32_t *itemIDs = dsCtx_[serverNum]->getLast20OrdersIndexResponseMessage()->getRegion()->itemIDs;

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. # distinct items = " << orderlinesCnt);

	// if there is no order, no further work is needed. just commit.
	if (orderlinesCnt == 0) {
		trxResult.result = TransactionResult::Result::COMMITTED;
		return trxResult;
	}

	bool signaled = false;
	for (size_t i=0; i < orderlinesCnt; i++){
		if (i == orderlinesCnt - 1) signaled = true;
		executor_.retrieveStock(i, itemIDs[i], cart_.wID, *localMemory_->getStockHead(), false);
		executor_.retrieveStockPointerList(i, itemIDs[i], cart_.wID, *localMemory_->getStockTS(), signaled);
	}
	executor_.synchronizeSendEvents();


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************
	bool abortFlag = false;
	unsigned retrieveOlderVersionCnt = 0;
	for (size_t i=0; i < orderlinesCnt; i++){
		if (! isRecordAccessible(localMemory_->getStockHead()->getRegion()[i].writeTimestamp)){
			int ind = findValidVersion(localMemory_->getStockTS()->getRegion(), config::tpcc_settings::VERSION_NUM);
			if (ind < 0) {
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Stock " << localMemory_->getStockHead()->getRegion()[i] << " and none of its versions are not consistent (locked or from a later snapshot)");
				abortFlag = true;
				break;

			}
			else{
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Stock " << localMemory_->getStockHead()->getRegion()[i] << " is not consistent"
						<< ", but its " << ind << "'s version is consistent (" << localMemory_->getStockTS()->getRegion()[ind] << ")");

				executor_.retrieveStockOlderVersion(i, itemIDs[i], cart_.wID, (size_t)ind, *localMemory_->getStockHead(), true);
				retrieveOlderVersionCnt++;
			}
		}
	}

	executor_.synchronizeSendEvents();

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
		if (localMemory_->getStockHead()->getRegion()[i].stock.S_QUANTITY < cart_.threshold)
			lowStockCnt++;
	}
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": The number of stocks with low quantity: " << (int)lowStockCnt);

	trxResult.result = TransactionResult::Result::COMMITTED;
	return trxResult;
}

} /* namespace TPCC */
