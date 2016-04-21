/*
 * DeliveryTransaction.cpp
 *
 *  Created on: Apr 14, 2016
 *      Author: erfanz
 */

#include "DeliveryTransaction.hpp"
#include "../../../../../util/utils.hpp"

#define CLASS_NAME "DeliveryTrx"

namespace TPCC {
DeliveryTransaction::DeliveryTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: BaseTransaction(os, "Delivery", executor, clientID, clientCnt, dsCtx, sessionState, random, context, oracleContext,localTimestampVector){
	localMemory_ 	= new DeliveryLocalMemory(os_, *context_);
}

DeliveryTransaction::~DeliveryTransaction() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localMemory_;
}

DeliveryCart DeliveryTransaction::buildCart(){
	DeliveryCart cart;

	// 2.7.1.1 For any given terminal, the home warehouse number (W_ID) is constant over the whole measurement interval.
	cart.wID = sessionState_->getHomeWarehouseID();

	// 2.7.1.2 The carrier number (O_CARRIER_ID) is random ly selected within [1 .. 10].
	cart.oCarrierID = (uint8_t) random_->number(1, 10);

	// 2.7.1.3 The delivery date (OL_DELIVERY_D) is generated within the SUT by using the current system date and time.
	cart.olDeliveryD = std::time(nullptr);
	return cart;
}

TPCC::TransactionResult DeliveryTransaction::doOne(){
	time_t timer;
	std::time(&timer);
	TransactionResult trxResult;
	TPCC::OrderVersion *orderV;
	TPCC::NewOrderVersion *newOrderV;
	TPCC::OrderLineVersion *orderLinesV;
	TPCC::CustomerVersion *customerV;

	// ************************************************
	//	Constructing the transaction cart
	// ************************************************
	DeliveryCart cart = buildCart();
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_  << ": Cart: " << cart);


	for (uint8_t dID = 0; dID < config::tpcc_settings::DISTRICT_PER_WAREHOUSE; dID++){
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Starting independent transaction for dID " << (int)dID);


		// ************************************************
		//	Acquire read timestamp
		// ************************************************
		executor_.getReadTimestamp(*localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP(), true);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.getReadTimestamp()
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": received read snapshot from oracle: " << readTimestampToString());


		// ************************************************
		//	Read records in read-set
		// ************************************************
		// The row in the NEW-ORDER table with matching NO_W_ID (equals W_ID) and NO_D_ID (equals D_ID) and with the lowest NO_O_ID value is selected.
		// This is the oldest undelivered order of that district.
		TPCC::ServerContext *serverCtx = getServerContext(cart.wID);
		executor_.getOldestUndeliveredOrder(
				clientID_,
				cart.wID,
				dID,
				*serverCtx->getIndexRequestMessage(),
				*serverCtx->getOldestUndeliveredOrderIndexResponseMessage(),
				serverCtx->getQP(),
				true);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: Oldest_Undelivered_Order. Parameters: wID = " << (int)cart.wID << ", dID = " << (int)dID);

		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.getOldestUndeliveredOrder()
		TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));	// for executor_.getOldestUndeliveredOrder()

		// check if there is an undelivered order in this district
		TPCC::OldestUndeliveredOrderIndexResMsg *oldestUndeliveredOrderRes = serverCtx->getOldestUndeliveredOrderIndexResponseMessage()->getRegion();
		if (! oldestUndeliveredOrderRes->isSuccessful || !oldestUndeliveredOrderRes->existUndeliveredOrder ) {
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": No undelivered order in wID: " << cart.wID << " and dID: " << (int)dID);
			continue;
		}

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index response message received. Type: Oldest_Undelivered_Order. Parameters: oID = " << (int)oldestUndeliveredOrderRes->oID
				<< ", #orderlines = " << (int)oldestUndeliveredOrderRes->numOfOrderlines);

		// From Order table, retrieve the row with matching O_W_ID, O_D_ID and O_ID.
		executor_.retrieveOrder(
				oldestUndeliveredOrderRes->clientWhoOrdered,
				oldestUndeliveredOrderRes->orderRegionOffset,
				cart.wID,
				*localMemory_->getOrderHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions,
				serverCtx->getQP(),
				false);
		orderV = localMemory_->getOrderHead()->getRegion();


		// From NewOrder table, retrieve the row with matching NO_W_ID, NO_D_ID and NO_ID.
		executor_.retrieveNewOrder(
				oldestUndeliveredOrderRes->clientWhoOrdered,
				oldestUndeliveredOrderRes->newOrderRegionOffset,
				cart.wID,
				*localMemory_->getNewOrderHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
				serverCtx->getQP(),
				false);
		newOrderV = localMemory_->getNewOrderHead()->getRegion();


		// From OrderLine table, retrieve the row with matching OL_W_ID, OL_D_ID and OL_ID.
		executor_.retrieveOrderLines(
				oldestUndeliveredOrderRes->clientWhoOrdered,
				cart.wID,
				oldestUndeliveredOrderRes->orderLineRegionOffset,
				oldestUndeliveredOrderRes->numOfOrderlines,
				*localMemory_->getOrderLineHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
				serverCtx->getQP(),
				true);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.retrieveOrderLines()
		orderLinesV = localMemory_->getOrderLineHead()->getRegion();


		// From Customer table, retrieve the customer with matching wID, dID and cID
		uint32_t cID = localMemory_->getOrderHead()->getRegion()->order.O_C_ID;
		executor_.retrieveCustomer(
				cart.wID,
				dID,
				cID,
				*localMemory_->getCustomerHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
				serverCtx->getQP(),
				false);
		customerV = localMemory_->getCustomerHead()->getRegion();

		executor_.retrieveCustomerPointerList(
				cart.wID,
				dID,
				cID,
				*localMemory_->getCustomerTS(),
				getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList,
				getServerContext(cart.wID)->getQP(),
				true);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.retrieveCustomer()


		// printing for debugging purposes
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved NewOrder: " << *newOrderV);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Order: " << *orderV);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved Customer " << *customerV);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved " << (int)oldestUndeliveredOrderRes->numOfOrderlines << " OrderLines ");
		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++)
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[READ] Client " << clientID_ << ": retrieved OrderLine " << orderLinesV[olNumber]);


		// ************************************************
		//	Check whether fetched items are from a consistent snapshot, and not locked
		// ************************************************
		bool abortFlag = false;
		if (! isRecordAccessible(orderV->writeTimestamp)){
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": Order " << *orderV << " is not consistent (locked or from a later snapshot)");
		}
		else if (! isRecordAccessible(newOrderV->writeTimestamp)){
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": NewOrder " << *newOrderV << " is not consistent (locked or from a later snapshot)");
		}

		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++) {
			if (! isRecordAccessible(orderLinesV[olNumber].writeTimestamp)){
				abortFlag = true;
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": item " << orderLinesV[olNumber] << " is not consistent (locked or from a later snapshot)");
				break;
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

				executor_.retrieveCustomerOlderVersion(
						cart.wID,
						dID,
						cID,
						(size_t)ind,
						*localMemory_->getCustomerHead(),
						getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->customerTableOlderVersions,
						getServerContext(cart.wID)->getQP(),
						true);
				TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.retrieveCustomerOlderVersion()

				if (! isRecordAccessible(customerV->writeTimestamp)) {
					DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": the newly read version is also inconsistent (customer: " << customerV->writeTimestamp << ")");
					abortFlag = true;
				}
			}
		}


		if (abortFlag == true) {
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
			trxResult.result = TransactionResult::Result::ABORTED;
			trxResult.reason = TransactionResult::Reason::INCONSISTENT_SNAPSHOT;
			continue;
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
		bool lockStatus = true;
		bool deletedStatus = false;

		primitive::version_offset_t	versionOffset;

		versionOffset = orderV->writeTimestamp.getVersionOffset();
		Timestamp orderLockTS (deletedStatus, lockStatus, versionOffset, clientID_, cts);
		executor_.lockOrder(
				*orderV,
				oldestUndeliveredOrderRes->clientWhoOrdered,
				oldestUndeliveredOrderRes->orderRegionOffset,
				cart.wID,
				orderLockTS,
				*localMemory_->getOrderLockRegion(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions,
				serverCtx->getQP(),
				false);

		versionOffset = newOrderV->writeTimestamp.getVersionOffset();
		Timestamp newOrderLockTS (deletedStatus, lockStatus, versionOffset, clientID_, cts);
		executor_.lockNewOrder(
				*newOrderV,
				oldestUndeliveredOrderRes->clientWhoOrdered,
				oldestUndeliveredOrderRes->newOrderRegionOffset,
				cart.wID,
				newOrderLockTS,
				*localMemory_->getNewOrderLockRegion(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
				serverCtx->getQP(),
				false);

		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++) {
			versionOffset = orderLinesV[olNumber].writeTimestamp.getVersionOffset();
			Timestamp orderLineLockTS (deletedStatus, lockStatus, versionOffset, clientID_, cts);
			executor_.lockOrderLine(
					olNumber,
					orderLinesV[olNumber],
					oldestUndeliveredOrderRes->clientWhoOrdered,
					oldestUndeliveredOrderRes->orderLineRegionOffset + olNumber,
					cart.wID,
					orderLineLockTS,
					*localMemory_->getOrderLinesLocksRegion(),
					serverCtx->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
					serverCtx->getQP(),
					false);
		}

		versionOffset = customerV->writeTimestamp.getVersionOffset();
		Timestamp customerLockTS (deletedStatus, lockStatus, versionOffset, clientID_, cts);
		executor_.lockCustomer(
				*customerV,
				customerLockTS,
				*localMemory_->getCustomerLockRegion(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
				serverCtx->getQP(),
				true);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// for executor_.lockCustomer()

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": received the results for all the locks");


		// Byte swapping, since values read by atomic operations are in Big Endian order
		Timestamp orderExistingLock;
		Timestamp newOrderExistingLock;
		std::vector<Timestamp> orderlineExistingLocks(oldestUndeliveredOrderRes->numOfOrderlines);
		Timestamp customerExistingLock;

		*localMemory_->getOrderLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getOrderLockRegion()->getRegion());
		orderExistingLock.copy(*localMemory_->getOrderLockRegion()->getRegion());

		*localMemory_->getNewOrderLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getNewOrderLockRegion()->getRegion());
		newOrderExistingLock.copy(*localMemory_->getNewOrderLockRegion()->getRegion());


		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++){
			localMemory_->getOrderLinesLocksRegion()->getRegion()[olNumber] = utils::bigEndianToHost(localMemory_->getOrderLinesLocksRegion()->getRegion()[olNumber]);
			orderlineExistingLocks[olNumber].copy(localMemory_->getOrderLinesLocksRegion()->getRegion()[olNumber]);
		}

		*localMemory_->getCustomerLockRegion()->getRegion() = utils::bigEndianToHost(*localMemory_->getCustomerLockRegion()->getRegion());
		customerExistingLock.copy(*localMemory_->getCustomerLockRegion()->getRegion());

		// checks if locks are successful
		abortFlag = false;
		bool isNewOrderChanged = false;
		bool isCustomerChanged = false;
		bool isLineOrderChanged = false;
		size_t largestChangedLineOrder = 0;

		if (! orderExistingLock.isEqual(orderV->writeTimestamp)) {
			abortFlag = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock Order " << orderV->order << " was NOT successful "
					<< "(expected: " << orderV->writeTimestamp << ", existing: " << orderExistingLock << ")");
		}

		if (! newOrderExistingLock.isEqual(newOrderV->writeTimestamp)) {
			abortFlag = true;
			isNewOrderChanged = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock NewOrder " << newOrderV->newOrder << " was NOT successful "
					<< "(expected: " << newOrderV->writeTimestamp << ", existing: " << newOrderExistingLock << ")");
		}

		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++){
			if (! orderlineExistingLocks[olNumber].isEqual(orderLinesV[olNumber].writeTimestamp)) {
				abortFlag = true;
				isLineOrderChanged = true;
				largestChangedLineOrder = olNumber;
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock OrderLine " << orderV->order << " was NOT successful "
						<< "(expected: " << orderLinesV[olNumber].writeTimestamp << ", existing: " << orderlineExistingLocks[olNumber] << ")");
			}
		}

		if (! customerExistingLock.isEqual(customerV->writeTimestamp)) {
			abortFlag = true;
			isCustomerChanged = true;
			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[CMSW] Client " << clientID_ << ": attempt to lock Customer " << customerV->customer << " was NOT successful "
					<< "(expected: " << customerV->writeTimestamp << ", existing: " << customerExistingLock << ")");
		}

		if (abortFlag){
			// some locks couldn't get acquired. must first release the successful ones, then abort
			size_t successfulLocks = 0;
			bool signaled;
			if (orderExistingLock.isEqual(orderV->writeTimestamp)) {
				successfulLocks++;

				// since all lock_revert operations go to the same queue pair, only one signaled operation is enough
				if (isNewOrderChanged || isLineOrderChanged || isCustomerChanged) signaled = false;
				else signaled = true;
				executor_.revertOrderLock(
						oldestUndeliveredOrderRes->clientWhoOrdered,
						oldestUndeliveredOrderRes->orderRegionOffset,
						cart.wID,
						*localMemory_->getOrderHead(),
						serverCtx->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions,
						serverCtx->getQP(),
						signaled);
			}

			if (newOrderExistingLock.isEqual(newOrderV->writeTimestamp)) {
				successfulLocks++;

				// since all lock_revert operations go to the same queue pair, only one signaled operation is enough
				if (isLineOrderChanged || isCustomerChanged) signaled = false;
				else signaled = true;
				executor_.revertNewOrderLock(
						oldestUndeliveredOrderRes->clientWhoOrdered,
						oldestUndeliveredOrderRes->newOrderRegionOffset,
						cart.wID,
						*localMemory_->getNewOrderHead(),
						serverCtx->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
						serverCtx->getQP(),
						signaled);
			}

			for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++){
				if (orderlineExistingLocks[olNumber].isEqual(orderLinesV[olNumber].writeTimestamp)) {
					successfulLocks++;

					if (olNumber != largestChangedLineOrder || isCustomerChanged) signaled = false;
					else signaled = true;
					executor_.revertOrderLineLock(
							olNumber,
							oldestUndeliveredOrderRes->clientWhoOrdered,
							oldestUndeliveredOrderRes->orderLineRegionOffset + olNumber,
							cart.wID,
							*localMemory_->getOrderLineHead(),
							serverCtx->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
							serverCtx->getQP(),
							signaled);
				}
			}

			if (customerExistingLock.isEqual(customerV->writeTimestamp)) {
				successfulLocks++;
				executor_.revertCustomerLock(
						*localMemory_->getCustomerHead(),
						serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
						serverCtx->getQP(),
						true);
			}

			if (successfulLocks > 0) {
				TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
				DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": reverted lock");
			}

			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": could not acquire lock on all items --> ** ABORT **");
			trxResult.result = TransactionResult::Result::ABORTED;
			trxResult.reason = TransactionResult::Reason::UNSUCCESSFUL_LOCK;
			//return trxResult;
			continue;
		}
		else DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully acquired all locks");


		// ************************************************
		//	Append old version to the versions list, and update the pointers list
		// ************************************************
		executor_.updateCustomerPointers(
				*customerV,
				*localMemory_->getCustomerTS(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList,
				serverCtx->getQP(),
				false);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated pointers for customer" << customerV->customer);


		executor_.updateCustomerOlderVersions(
				//*localMemory_->getCustomerOlderVersions(),
				*localMemory_->getCustomerHead(),	// using CustomerHead instead of CustomerOlderVersions avoids redundant copying
				serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableOlderVersions,
				serverCtx->getQP(),
				false);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Writ] Client " << clientID_ << ": updated older versions for customer" << customerV->customer);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": added the pointers and older version");


		// ************************************************
		// Insert and unlock new records in write-set
		// ************************************************
		// update district (increment the next available order number D_NEXT_O_ID)
		lockStatus = false;

		// 2.7.4.2  update the order's O_CARRIER_ID in Order table.
		deletedStatus = false;
		versionOffset = orderV->writeTimestamp.getVersionOffset();
		orderV->writeTimestamp.setAll(deletedStatus, lockStatus, versionOffset, clientID_, cts);
		orderV->order.O_CARRIER_ID = cart.oCarrierID;

		executor_.insertIntoOrder(
				oldestUndeliveredOrderRes->clientWhoOrdered,
				oldestUndeliveredOrderRes->orderRegionOffset,
				cart.wID,
				*localMemory_->getOrderHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions,
				serverCtx->getQP(),
				false);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated Order " << orderV->order);


		// 2.7.4.2 The selected row in the NEW-ORDER table is deleted
		deletedStatus = true;
		versionOffset = newOrderV->writeTimestamp.getVersionOffset();
		newOrderV->writeTimestamp.setAll(deletedStatus, lockStatus, versionOffset, clientID_, cts);
		newOrderV->writeTimestamp.markDeleted();

		executor_.insertIntoNewOrder(
				clientID_,
				nextNewOrderID_,
				cart.wID,
				*localMemory_->getNewOrderHead(),
				getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions,
				getServerContext(cart.wID)->getQP(),
				false);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": deleted NewOrder " << newOrderV->newOrder);


		// 2.7.4.2 All OL_DELIVERY_D, the delivery dates, are updated to the current system time as returned by the operating system and the sum of all OL_AMOUNT is retrieved.
		float sumOfOrderLinesAmount = 0;
		deletedStatus = false;
		for (size_t olNumber = 0; olNumber < oldestUndeliveredOrderRes->numOfOrderlines; olNumber++){
			versionOffset = orderLinesV[olNumber].writeTimestamp.getVersionOffset();
			orderLinesV[olNumber].writeTimestamp.setAll(deletedStatus, lockStatus, versionOffset, clientID_, cts);
			orderLinesV[olNumber].orderLine.OL_DELIVERY_D = cart.olDeliveryD;
			sumOfOrderLinesAmount += orderLinesV[olNumber].orderLine.OL_AMOUNT;

			executor_.insertIntoOrderLine(
					oldestUndeliveredOrderRes->clientWhoOrdered,
					oldestUndeliveredOrderRes->orderLineRegionOffset + olNumber,
					(uint8_t)olNumber,
					cart.wID,
					*localMemory_->getOrderLineHead(),
					getServerContext(cart.wID)->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions,
					getServerContext(cart.wID)->getQP(),
					false);

			DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated OrderLine " << orderLinesV[olNumber].orderLine << " (" << (int)olNumber << ")");
		}

		// 2.7.4.2  C_BALANCE is increased by the sum of all order-line amounts (OL_AMOUNT) previously retrieved. C_DELIVERY_CNT is incremented by 1.
		deletedStatus = false;
		versionOffset = customerV->writeTimestamp.getVersionOffset();
		customerV->writeTimestamp.setAll(deletedStatus, lockStatus, versionOffset, clientID_, cts);
		customerV->customer.C_BALANCE = customerV->customer.C_BALANCE + sumOfOrderLinesAmount;
		customerV->customer.C_DELIVERY_CNT = (uint16_t)(customerV->customer.C_DELIVERY_CNT + 1);

		executor_.updateCustomer(
				*localMemory_->getCustomerHead(),
				serverCtx->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions,
				serverCtx->getQP(),
				true);

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": updated Customer" << customerV->customer);

		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));	// executor_.insertIntoOrderLine()

		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Client " << clientID_ << ": successfully installed and unlocked all records");

		// update the index
		executor_.registerDelivery(
				clientID_,
				cart.wID,
				dID,
				orderV->order.O_ID,
				*serverCtx->getIndexRequestMessage(),
				*serverCtx->getRegisterDeliveryIndexResponseMessage(),
				serverCtx->getQP(),
				true);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Send] Client " << clientID_ << ": Index Request Message sent. Type: REGISTER_DELIVERY. Parameters: wID = " << (int)cart.wID
				<< ", dID = " << (int)dID << ", OID = " << (int)orderV->order.O_ID);

		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

		// receive the acknowledgement of the index update request
		TEST_NZ (RDMACommon::poll_completion(context_->getRecvCq()));
		assert(serverCtx->getRegisterOrderIndexResponseMessage()->getRegion()->isSuccessful == true);
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Recv] Client " << clientID_ << ": Index Response Message received for message . Type = REGISTER_DELIVERY");

		// ************************************************
		//	Submit the result to the oracle
		// ************************************************
		trxResult.result = TransactionResult::Result::COMMITTED;
		trxResult.reason = TransactionResult::Reason::SUCCESS;
		trxResult.cts = cts;
		localTimestampVector_->getRegion()[clientID_] = trxResult.cts;
		executor_.submitResult(clientID_, *localTimestampVector_, oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector, oracleContext_->getQP(), true);
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_WRITE(os_, CLASS_NAME, __func__, "[WRIT] Client " << clientID_ << ": sent trx result for CTS " << cts << " to the Oracle");
	}

	trxResult.result = TransactionResult::Result::COMMITTED;
	return trxResult;
}

} /* namespace TPCC */