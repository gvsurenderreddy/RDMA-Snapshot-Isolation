/*
 * OrderStatusTransaction.cpp
 *
 *  Created on: Apr 4, 2016
 *      Author: erfanz
 */

#include "OrderStatusTransaction.hpp"
#include "../../../../../util/utils.hpp"

#define CLASS_NAME "OrdStatTrx"
namespace TPCC {


OrderStatusTransaction::OrderStatusTransaction(TPCCClient &client, DBExecutor &executor)
: BaseTransaction("OrderStatus", client, executor){
	localMemory_ 	= new OrderStatusLocalMemory(os_, context_);
}

OrderStatusTransaction::~OrderStatusTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

void OrderStatusTransaction::buildCart(){
	// reset cart
	cart_.reset();

	// 2.6.1.1 For any given terminal, the home warehouse number (W_ID) is constant over the whole measurement interval
	cart_.wID = sessionState_.getHomeWarehouseID();

	// 2.6.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart_.dID = (uint8_t) random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.6.1.2 The customer is randomly selected 60% of the time by last name (C_W_ID , C_D_ID, C_LAST) and 40% of the time by number
	int y = random_.number(1, 100);
	if (y <= 60){
		// selection by LastName
		cart_.customerSelectionMode = OrderStatusCart::LAST_NAME;
		random_.lastName(cart_.cLastName, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
	}
	else {
		// selection by ID
		cart_.customerSelectionMode = OrderStatusCart::ID;
		cart_.cID = (uint32_t) random_.NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
	}
}

void OrderStatusTransaction::initilizeTransaction(){
	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart);
}

TPCC::TransactionResult OrderStatusTransaction::doOne(){
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
	size_t serverNum = Warehouse::getServerNum(cart_.wID);


	if (cart_.customerSelectionMode == OrderStatusCart::LAST_NAME) {
		// First, use the index on the server to find the cID for the given last name
		executor_.lookupCustomerByLastName(
				clientID_,
				cart_.wID,
				cart_.dID,
				cart_.cLastName,
				*dsCtx_[serverNum]->getIndexRequestMessage(),
				*dsCtx_[serverNum]->getCustomerNameIndexResponseMessage(),
				dsCtx_[serverNum]->getQP(),
				true);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: LastName_TO_CID. Parameters: wID = " << (int)cart_.wID << ", dID = " << (int)cart_.dID << ", lastName = " << cart_.cLastName);

		executor_.synchronizeNetworkEvents();

		TPCC::CustomerNameIndexRespMsg *customerLastNameRes = dsCtx_[serverNum]->getCustomerNameIndexResponseMessage()->getRegion();
		assert(customerLastNameRes->isSuccessful == true);
		cart_.cID = customerLastNameRes->cID;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. cID = " << (int)cart_.cID);
	}

	// get the largest existing O_ID for (O_W_ID = cart_.wID, O_D_ID = cart_.dID, O_C_ID = cart_.CID).
	executor_.getLastOrderOfCustomer(
			clientID_,
			cart_.wID,
			cart_.dID,
			cart_.cID,
			*dsCtx_[serverNum]->getIndexRequestMessage(),
			*dsCtx_[serverNum]->getLargestOrderForCustomerIndexResponseMessage(),
			dsCtx_[serverNum]->getQP(),
			true);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: LARGEST_ORDER_FOR_CUSTOMER. Parameters: wID = " << (int)cart_.wID << ", dID = " << (int)cart_.dID << ", cID = " << cart_.cID);
	executor_.synchronizeNetworkEvents();

	TPCC::LargestOrderForCustomerIndexRespMsg *largestOrderRes = dsCtx_[serverNum]->getLargestOrderForCustomerIndexResponseMessage()->getRegion();
	if (largestOrderRes->isSuccessful == false){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. Customer has no register order. Therefore, COMMITs without any further action.");
		trxResult.result = TransactionResult::Result::COMMITTED;
		return trxResult;
	}

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index Response Message received. oID = " << (int)largestOrderRes->oID << ", client_who_ordered: " << (int)largestOrderRes->clientWhoOrdered
			<< ", order_region_offset: " << (int)largestOrderRes->orderRegionOffset << ", orderLine_region_offset: " << (int)largestOrderRes->orderLineRegionOffset << ", #orderlines: " << (int)largestOrderRes->numOfOrderlines);

	// retrieve the orderlines
	executor_.retrieveOrderLines(
			largestOrderRes->clientWhoOrdered,
			cart_.wID,
			largestOrderRes->orderRegionOffset,
			largestOrderRes->numOfOrderlines,
			*localMemory_->getOrderLineHead(),
			true);

	// wait for all outstanding RDMA requests to finish
	executor_.synchronizeSendEvents();

	if (config::SNAPSHOT_ACQUISITION_TYPE == config::SnapshotAcquisitionType::ONLY_READ_SET){
		// ************************************************
		//	Acquire Partial read timestamp
		// ************************************************

		// First find which clients are needed to put in the snapshot
		for (uint8_t olNumber = 0; olNumber < largestOrderRes->numOfOrderlines; olNumber++){
			oracleContext_.insertClientIDIntoSnapshot(localMemory_->getOrderLineHead()->getRegion()[olNumber].writeTimestamp.getClientID());
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

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved " << (int)largestOrderRes->numOfOrderlines << " lineitems for order with oID = " << (int)largestOrderRes->oID);


	// ************************************************
	//	Check whether fetched items are from a consistent snapshot, and not locked
	// ************************************************

	bool abortFlag = false;
	for (uint8_t olNumber = 0; olNumber < largestOrderRes->numOfOrderlines; olNumber++){
		if (! isRecordAccessible(localMemory_->getOrderLineHead()->getRegion()[olNumber].writeTimestamp)){
			// TODO: fetch older versions if the head version is inconsistent.
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": OrderLine #" << (int)olNumber << ": "
					<< localMemory_->getOrderLineHead()->getRegion()[olNumber] << " is not consistent (locked or from a later snapshot) --> ** ABORT **");
			break;
		}
	}
	// ************************************************
	//	Submit the result to the oracle
	// ************************************************
	if (abortFlag) {
		trxResult.result = TransactionResult::Result::ABORTED;
		trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
	}
	else trxResult.result = TransactionResult::Result::COMMITTED;
	return trxResult;
}

} /* namespace TPCC */
