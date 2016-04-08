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

OrderStatusTransaction::OrderStatusTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: BaseTransaction(os, "OrderStatus", executor, clientID, clientCnt, dsCtx, sessionState, random, context, oracleContext,localTimestampVector){
	localMemory_ 	= new OrderStatusLocalMemory(os_, *context_);
}

OrderStatusTransaction::~OrderStatusTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

TPCC::OrderStatusCart OrderStatusTransaction::buildCart(){
	OrderStatusCart cart;

	// 2.6.1.1 For any given terminal, the home warehouse number (W_ID) is constant over the whole measurement interval
	cart.wID = sessionState_->getHomeWarehouseID();

	// 2.6.1.2 The district number (D_ID) is randomly selected within [1 .. 10] from the home warehouse (D_W_ID = W_ID).
	cart.dID = (uint8_t) random_->number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	// 2.6.1.2 The customer is randomly selected 60% of the time by last name (C_W_ID , C_D_ID, C_LAST) and 40% of the time by number
	int y = random_->number(1, 100);
	if (y <= 60){
		// selection by LastName
		cart.customerSelectionMode = OrderStatusCart::LAST_NAME;
		random_->lastName(cart.cLastName, config::tpcc_settings::CUSTOMER_PER_DISTRICT);
	}
	else {
		// selection by ID
		cart.customerSelectionMode = OrderStatusCart::ID;
		cart.cID = (uint32_t) random_->NURand(1023, 0, config::tpcc_settings::CUSTOMER_PER_DISTRICT - 1);
	}

	return cart;
}

TPCC::TransactionResult OrderStatusTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;


	// ************************************************
	//	Constructing the shopping cart
	// ************************************************
	OrderStatusCart cart = buildCart();
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

	if (cart.customerSelectionMode == OrderStatusCart::LAST_NAME) {
		// First, use the index on the server to find the cID for the given last name
		executor_.lookupCustomerByLastName(
				clientID_,
				cart.wID,
				cart.dID,
				cart.cLastName,
				*serverCtx->getIndexRequestMessage(),
				*serverCtx->getIndexResponseMessage(),
				serverCtx->getQP(),
				true);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: LastName_TO_CID. Parameters: wID = " << (int)cart.wID << ", dID = " << (int)cart.dID << ", lastName = " << cart.cLastName);

		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.lookupCustomerByLastName()


		TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));
		assert(serverCtx->getIndexResponseMessage()->getRegion()->isSuccessful == true);
		cart.cID = serverCtx->getIndexResponseMessage()->getRegion()->result.lastNameIndex.cID;
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. cID = " << (int)cart.cID);
	}

	// get the largest existing O_ID for (O_W_ID = cart.wID, O_D_ID = cart.dID, O_C_ID = cart.CID).
	executor_.getLastOrderOfCustomer(
			clientID_,
			cart.wID,
			cart.dID,
			cart.cID,
			*serverCtx->getIndexRequestMessage(),
			*serverCtx->getIndexResponseMessage(),
			serverCtx->getQP(),
			true);

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: LARGEST_ORDER_FOR_CUSTOMER. Parameters: wID = " << (int)cart.wID << ", dID = " << (int)cart.dID << ", cID = " << cart.cID);
	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

	TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));

	if (serverCtx->getIndexResponseMessage()->getRegion()->isSuccessful == false){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received. Customer has no register order. Therefore, COMMITs without any further action.");
		trxResult.result = TransactionResult::Result::COMMITTED;
		return trxResult;
	}

	uint32_t oID = serverCtx->getIndexResponseMessage()->getRegion()->result.largestOrderIndex.oID;
	primitive::client_id_t clientWhoOrdered = serverCtx->getIndexResponseMessage()->getRegion()->result.largestOrderIndex.clientWhoOrdered;
	size_t orderRegionOffset = serverCtx->getIndexResponseMessage()->getRegion()->result.largestOrderIndex.orderRegionOffset;
	size_t orderLineRegionOffset = serverCtx->getIndexResponseMessage()->getRegion()->result.largestOrderIndex.orderLineRegionOffset;
	uint8_t numOfOrderlines = serverCtx->getIndexResponseMessage()->getRegion()->result.largestOrderIndex.numOfOrderlines;
	(void)oID;	// to get rid of the "unused variable" warning
	(void)orderRegionOffset;	// to get rid of the "unused variable" warning

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Index Response Message received. oID = " << (int)oID << ", client_who_ordered: " << (int)clientWhoOrdered
			<< ", order_region_offset: " << (int)orderRegionOffset << ", orderLine_region_offset: " << (int)orderLineRegionOffset << ", #orderlines: " << (int)numOfOrderlines);


	// retrieve the orderlines
	executor_.retrieveOrderLines(
			clientWhoOrdered,
			cart.wID,
			orderLineRegionOffset,
			numOfOrderlines,
			*localMemory_->getOrderLineHead(),
			getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
			getServerContext(cart.wID)->getQP(),
			true);

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved " << (int)numOfOrderlines << " lineitems for order with oID = " << (int)oID);


	trxResult.result = TransactionResult::Result::COMMITTED;
	return trxResult;
}

} /* namespace TPCC */