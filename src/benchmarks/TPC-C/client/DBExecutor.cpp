/*
 * DBExecutor.cpp
 *
 *  Created on: Mar 13, 2016
 *      Author: erfanz
 */

#include "DBExecutor.hpp"
#include "../../../util/utils.hpp"
#include "../../../../config.hpp"


#define CLASS_NAME "DBExecutor"


namespace TPCC {

DBExecutor::~DBExecutor() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Destructor called");
}

DBExecutor::DBExecutor(std::ostream &os, std::vector<ServerContext *> dsCtx, unsigned instanceNum, ibv_cq *sendCompletionQueue, ibv_cq *recvCompletionQueue)
: os_(os),
  dsCtx_(dsCtx),
  instanceNum_(instanceNum),
  sendCQ_(sendCompletionQueue),
  recvCQ_(recvCompletionQueue),
  outstandingSendCompletionCnt_(0),
  outstandingRecvCompletionCnt_(0){
}


void DBExecutor::synchronizeSendEvents(){
	if (outstandingSendCompletionCnt_ != 0) {
		for (size_t i = 0; i < outstandingSendCompletionCnt_; i++)
			TEST_NZ (RDMACommon::poll_completion(sendCQ_));
		outstandingSendCompletionCnt_ = 0;
	}
}

void DBExecutor::synchronizeRecvEvents(){
	if (outstandingRecvCompletionCnt_ != 0) {
		for (size_t i = 0; i < outstandingRecvCompletionCnt_; i++)
			TEST_NZ (RDMACommon::poll_completion(recvCQ_));
		outstandingRecvCompletionCnt_ = 0;
	}
}

void DBExecutor::synchronizeNetworkEvents(){
	synchronizeSendEvents();
	synchronizeRecvEvents();
}

bool DBExecutor::isServerLocal(size_t serverNum) const {
	return (config::LOCALITY_EXPLOITAION && dsCtx_[serverNum]->getInstanceNum());
}

void DBExecutor::lookupCustomerByLastName(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, const char *cLastName, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::CustomerNameIndexRespMsg> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::LOOKUP;
	req->indexType = TPCC::IndexRequestMessage::IndexType::CUSTOMER_LAST_NAME_INDEX;
	req->parameters.lastNameIndex.warehouseOffset = warehouseOffset;
	req->parameters.lastNameIndex.dID = dID;
	std::memcpy(req->parameters.lastNameIndex.customerLastName, cLastName,  17);

	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(CustomerNameIndexRespMsg)));

	outstandingRecvCompletionCnt_++;

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}

void DBExecutor::getLastOrderOfCustomer(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::LOOKUP;
	req->indexType = TPCC::IndexRequestMessage::IndexType::LARGEST_ORDER_FOR_CUSTOMER_INDEX;
	req->parameters.largestOrderIndex.warehouseOffset = warehouseOffset;
	req->parameters.largestOrderIndex.dID = dID;
	req->parameters.largestOrderIndex.cID= cID;

	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(LargestOrderForCustomerIndexRespMsg)));
	outstandingRecvCompletionCnt_++;

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}

void DBExecutor::registerOrder(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, uint32_t cID, uint32_t oID, size_t orderRegionOffset, size_t newOrderRegionOffset, size_t orderLineRegionOffset, uint8_t numOfOrderlines, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::IndexResponseMessage> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::UPDATE;
	req->indexType = TPCC::IndexRequestMessage::IndexType::REGISTER_ORDER;
	req->parameters.registerOrderIndex.warehouseOffset = warehouseOffset;
	req->parameters.registerOrderIndex.dID = dID;
	req->parameters.registerOrderIndex.cID = cID;
	req->parameters.registerOrderIndex.oID = oID;
	req->parameters.registerOrderIndex.orderRegionOffset = orderRegionOffset;
	req->parameters.registerOrderIndex.newOrderRegionOffset = newOrderRegionOffset;
	req->parameters.registerOrderIndex.orderLineRegionOffset = orderLineRegionOffset;
	req->parameters.registerOrderIndex.numOfOrderlines = numOfOrderlines;


	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(IndexResponseMessage)));
	outstandingRecvCompletionCnt_++;


	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}

void DBExecutor::getDistinctItemsForLastTwentyOrders(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, uint32_t D_NEXT_O_ID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::Last20OrdersIndexResMsg> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::LOOKUP;
	req->indexType = TPCC::IndexRequestMessage::IndexType::ITEMS_FOR_LAST_20_ORDERS;
	req->parameters.last20OrdersIndex.warehouseOffset = warehouseOffset;
	req->parameters.last20OrdersIndex.dID = dID;
	req->parameters.last20OrdersIndex.D_NEXT_O_ID = D_NEXT_O_ID;

	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(Last20OrdersIndexResMsg)));
	outstandingRecvCompletionCnt_++;

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}

void DBExecutor::getOldestUndeliveredOrder(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::LOOKUP;
	req->indexType = TPCC::IndexRequestMessage::IndexType::OLDEST_UNDELIVERED_ORDER;
	req->parameters.oldestUndeliveredOrderIndex.warehouseOffset = warehouseOffset;
	req->parameters.oldestUndeliveredOrderIndex.dID = dID;

	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(OldestUndeliveredOrderIndexResMsg)));
	outstandingRecvCompletionCnt_++;

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}

void DBExecutor::registerDelivery(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, uint32_t oID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::IndexResponseMessage> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	TPCC::IndexRequestMessage *req = requestRegion.getRegion();
	req->clientID = clientID;
	req->operationType = TPCC::IndexRequestMessage::OperationType::UPDATE;
	req->indexType = TPCC::IndexRequestMessage::IndexType::REGISTER_DELIVERY;
	req->parameters.registerDeliveryIndex.warehouseOffset = warehouseOffset;
	req->parameters.registerDeliveryIndex.dID = dID;
	req->parameters.registerDeliveryIndex.oID = oID;


	// to avoid race, post the next indexResponse message before sending the request
	TEST_NZ (RDMACommon::post_RECEIVE (
			qp,
			responseRegion.getRDMAHandler(),
			(uintptr_t)responseRegion.getRegion(),
			sizeof(IndexResponseMessage)));
	outstandingRecvCompletionCnt_++;

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
	outstandingSendCompletionCnt_++;
}


void DBExecutor::getReadTimestamp(RDMARegion<primitive::timestamp_t> &localRegion, MemoryHandler<primitive::timestamp_t> &remoteMH, ibv_qp *qp, bool signaled) {
	primitive::timestamp_t *lookupAddress = (primitive::timestamp_t*)remoteMH.rdmaHandler_.addr;
	uint32_t size = (uint32_t)(remoteMH.regionSize_ * sizeof(primitive::timestamp_t));

	if(remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Error in timestamp acquisition");
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_READ,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			signaled));

	if (signaled) outstandingSendCompletionCnt_++;
}



void DBExecutor::submitResult(primitive::client_id_t clientID, RDMARegion<primitive::timestamp_t> &localRegion,  MemoryHandler<primitive::timestamp_t> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address of the timestamp
	size_t tableOffset = (size_t)(clientID * sizeof(primitive::timestamp_t));		// offset of client's cts in timestampVector
	primitive::timestamp_t *writeAddress = (primitive::timestamp_t*)(tableOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(primitive::timestamp_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: clientID_ = " << clientID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_WRITE,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[clientID],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}


void DBExecutor::retrieveWarehouse(uint16_t wID, RDMARegion<WarehouseVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)warehouseOffset;	// offset of WarehouseVersion in WarehouseTable

	uint32_t size = (uint32_t) sizeof(WarehouseVersion);	// Size to be read from the remote side

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::WarehouseVersion *lookupAddress =  (WarehouseVersion*)(tableIndex * sizeof(TPCC::WarehouseVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID );
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveWarehouseOlderVersion(uint16_t wID, size_t versionOffset, RDMARegion<WarehouseVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM);	// offset of WarehouseVersion in WarehouseTable
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	uint32_t size = (uint32_t) sizeof(WarehouseVersion);	// Size to be read from the remote side

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[index], size);
	}
	else{
		// the data sits at a remote server
		TPCC::WarehouseVersion *lookupAddress =  (WarehouseVersion*)(index * sizeof(TPCC::WarehouseVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", versionOffset: " << (int)versionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveWarehousePointerList(uint16_t wID, RDMARegion<Timestamp> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableTimestampList;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM);

	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));	// Size to be read from the remote side

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else{
		// the data sits at a remote server
		Timestamp *lookupAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockWarehouse(TPCC::WarehouseVersion &warehouseV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	// The remote address of the timestamp
	uint16_t wID = warehouseV.warehouse.W_ID;
	uint64_t* oldTS_UUL = (uint64_t *) &warehouseV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	size_t timestampOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfTimestamp();		// offset of Timestamp in WarehouseVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_UUL,
			*newTS_UUL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertWarehouseLock(RDMARegion<TPCC::WarehouseVersion> &localRegion, bool signaled){
	uint16_t wID = localRegion.getRegion()->warehouse.W_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)warehouseOffset;	// offset of WarehouseVersion in WarehouseTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()->writeTimestamp);
	}
	else{
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfTimestamp();	// offset of Timestamp in WarehouseVersion
		Timestamp *writeAddress = (Timestamp *)(tableIndex * sizeof(TPCC::WarehouseVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()->writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateWarehousePointers(TPCC::WarehouseVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled) {
	uint16_t wID = oldHead.warehouse.W_ID;
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1].copy(versionArray[i]);

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0].copy(oldHead.writeTimestamp);

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		Timestamp *writeAddress = (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)versionArray,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateWarehouseOlderVersions(RDMARegion<TPCC::WarehouseVersion> &localRegion, bool signaled) {
	TPCC::WarehouseVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->warehouse.W_ID;
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM);
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::WarehouseVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[index], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		WarehouseVersion *writeAddress =  (WarehouseVersion *)(index * sizeof(TPCC::WarehouseVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateWarehouse(RDMARegion<TPCC::WarehouseVersion> &localRegion, bool signaled){
	uint16_t wID = localRegion.getRegion()->warehouse.W_ID;
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::WarehouseVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->warehouseTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::WarehouseVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		TPCC::WarehouseVersion *writeAddress =  (TPCC::WarehouseVersion *)(tableIndex * sizeof(TPCC::WarehouseVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}






void DBExecutor::retrieveDistrict(uint16_t wID, uint8_t dID, RDMARegion<DistrictVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the district tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);	// offset of DistrictVersion in DistrictTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else{
		// the data sits at a remote server
		TPCC::DistrictVersion* lookupAddress =  (TPCC::DistrictVersion *)(tableIndex * sizeof(TPCC::DistrictVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveDistrictOlderVersion(uint16_t wID, uint8_t dID, size_t versionOffset, RDMARegion<DistrictVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM);	// offset of DistrictVersion in DistrictTable
	size_t circularBufferOffset = (size_t) versionOffset;
	size_t index = tableOffset + circularBufferOffset;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(DistrictVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[index], size);
	}
	else{
		// the data sits at a remote server
		TPCC::DistrictVersion *lookupAddress =  (DistrictVersion*)(index * sizeof(TPCC::DistrictVersion)  + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", versionOffset: " << (int)versionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveDistrictPointerList(uint16_t wID, uint8_t dID, RDMARegion<Timestamp> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableTimestampList;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else{
		// the data sits at a remote server
		Timestamp *lookupAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID, RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();


	// The remote address to read the district tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfNextOID();		// offset of D_NEXT_O_ID in District
	uint64_t *lookupAddress =  (uint64_t *)(tableOffset + districtOffset + fieldOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	// Size to be read from the remote side
	size_t size = sizeof(localRegion.getRegion()->district.D_NEXT_O_ID);
	assert(size == sizeof(uint64_t));

	if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&(localRegion.getRegion()->district.D_NEXT_O_ID),
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			(uint64_t)1ULL,
			(uint32_t)size,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::lockDistrict(TPCC::DistrictVersion &districtV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	uint16_t wID = districtV.district.D_W_ID;
	uint8_t dID = districtV.district.D_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint64_t* oldTS_UUL = (uint64_t *) &districtV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t timestampOffset = (size_t)TPCC::DistrictVersion::getOffsetOfTimestamp();		// offset of Timestamp in DistrictVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_UUL,
			*newTS_UUL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertDistrictLock(RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled){
	uint16_t wID = localRegion.getRegion()->district.D_W_ID;
	uint8_t dID = localRegion.getRegion()->district.D_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(localRegion.getRegion()->district.D_W_ID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);	// offset of DistrictVersion in DistrictTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()->writeTimestamp);
	}
	else{
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::DistrictVersion::getOffsetOfTimestamp();	// offset of Timestamp in DistrictVersion
		Timestamp *writeAddress = (Timestamp *)(tableIndex * sizeof(TPCC::DistrictVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()->writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateDistrictPointers(TPCC::DistrictVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled) {
	// The remote address to read the item info
	uint16_t wID = oldHead.district.D_W_ID;
	uint8_t dID = oldHead.district.D_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1].copy(versionArray[i]);

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0].copy(oldHead.writeTimestamp);

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM);	// offset of DistrictVersion in DistrictTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		Timestamp *writeAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)versionArray,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateDistrictOlderVersions(RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled) {
	TPCC::DistrictVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->district.D_W_ID;
	uint8_t dID = oldHead->district.D_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM);	// offset of DistrictVersion in DistrictTable
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[index], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		DistrictVersion *writeAddress =  (DistrictVersion *)(index * sizeof(TPCC::DistrictVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateDistrict(RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->district.D_W_ID;
	uint8_t dID = localRegion.getRegion()->district.D_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::DistrictVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->districtTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID);	// offset of DistrictVersion in DistrictTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		TPCC::DistrictVersion *writeAddress =  (TPCC::DistrictVersion *)(tableIndex * sizeof(TPCC::DistrictVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}




void DBExecutor::retrieveCustomer(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<CustomerVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions;
	ibv_qp* qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the customer info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID);  // offset of CustomerVersion in CustomerTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::CustomerVersion *lookupAddress =  (TPCC::CustomerVersion *)(tableIndex * sizeof(TPCC::CustomerVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveCustomerOlderVersion(uint16_t wID, uint8_t dID, uint32_t cID, size_t versionOffset, RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* config::tpcc_settings::VERSION_NUM;
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(CustomerVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[index], size);
	}
	else{
		// the data sits at a remote server
		TPCC::CustomerVersion *lookupAddress =  (CustomerVersion*)(index * sizeof(TPCC::CustomerVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID << ", versionOffset: " << (int)versionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveCustomerPointerList(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<Timestamp> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* config::tpcc_settings::VERSION_NUM;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else{
		// the data sits at a remote server
		Timestamp *lookupAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockCustomer(TPCC::CustomerVersion &customerV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	// The remote address of the timestamp
	uint16_t wID = customerV.customer.C_W_ID;
	uint8_t dID = customerV.customer.C_D_ID;
	uint32_t cID = customerV.customer.C_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint64_t* oldTS_UUL = (uint64_t *) &customerV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);

	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * sizeof(TPCC::CustomerVersion));
	size_t timestampOffset = (size_t)TPCC::CustomerVersion::getOffsetOfTimestamp();		// offset of Timestamp in CustomerVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << cID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_UUL,
			*newTS_UUL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertCustomerLock(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled){
	uint16_t wID = localRegion.getRegion()->customer.C_W_ID;
	uint8_t dID = localRegion.getRegion()->customer.C_D_ID;
	uint32_t cID = localRegion.getRegion()->customer.C_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID);	// offset of CustomerVersion in CustomerTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()->writeTimestamp);
	}
	else{
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::CustomerVersion::getOffsetOfTimestamp();		// offset of Timestamp in CustomerVersion
		Timestamp *writeAddress = (Timestamp *)(tableIndex * sizeof(TPCC::CustomerVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()->writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateCustomerPointers(TPCC::CustomerVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1].copy(versionArray[i]);

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0].copy(oldHead.writeTimestamp);

	// The remote address to read the item info
	uint16_t wID = oldHead.customer.C_W_ID;
	uint8_t dID = oldHead.customer.C_D_ID;
	uint32_t cID = oldHead.customer.C_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * config::tpcc_settings::VERSION_NUM;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		Timestamp *writeAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)versionArray,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateCustomerOlderVersions(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled) {
	TPCC::CustomerVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->customer.C_W_ID;
	uint8_t dID = oldHead->customer.C_D_ID;
	uint32_t cID = oldHead->customer.C_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * config::tpcc_settings::VERSION_NUM;
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[index], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		CustomerVersion *writeAddress =  (CustomerVersion *)(index * sizeof(TPCC::CustomerVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateCustomer(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->customer.C_W_ID;
	uint8_t dID = localRegion.getRegion()->customer.C_D_ID;
	uint32_t cID = localRegion.getRegion()->customer.C_ID;

	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::CustomerVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->customerTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else{
		// the data sits at a remote server
		TPCC::CustomerVersion *writeAddress =  (TPCC::CustomerVersion *)(tableIndex * sizeof(TPCC::CustomerVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID << ", cID = " << (int)cID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}




void DBExecutor::retrieveItem(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<ItemVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::ItemVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->itemTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	size_t tableIndex = (size_t)iID;		// offset of ItemVersion in ItemTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::ItemVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&localRegion.getRegion()[offsetInLocalRegion], &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::ItemVersion *lookupAddress =  (TPCC::ItemVersion *)(tableIndex * sizeof(TPCC::ItemVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: wID = " << (int)wID << ", iID = " << (int)iID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveStock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<StockVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID);		// offset of StockVersion in StockTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&localRegion.getRegion()[offsetInLocalRegion], &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::StockVersion *lookupAddress =  (TPCC::StockVersion *)(tableIndex * sizeof(TPCC::StockVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveStockOlderVersion(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, size_t versionOffset, RDMARegion<StockVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * config::tpcc_settings::VERSION_NUM);	// offset of StockVersion in StockTable
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(StockVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&localRegion.getRegion()[offsetInLocalRegion], &remoteMH.region_[index], size);
	}
	else{
		// the data sits at a remote server
		TPCC::StockVersion *lookupAddress =  (StockVersion*)(index * sizeof(TPCC::StockVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", versionOffset: " << (int)versionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveStockPointerList(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<Timestamp> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * config::tpcc_settings::VERSION_NUM);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	size_t offset = (size_t) offsetInLocalRegion * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&localRegion.getRegion()[offset], &remoteMH.region_[tableIndex], size);
	}
	else{
		// the data sits at a remote server
		Timestamp *lookupAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offset],
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockStock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)offsetInLocalRegion);
		exit(-1);
	}

	uint64_t *oldTS_ULL = (uint64_t *)&oldTS;
	uint64_t *newTS_ULL = (uint64_t *)&newTS;

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_ULL,
			*newTS_ULL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertStockLock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID);		// offset of StockVersion in StockTable
	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()[offsetInLocalRegion].writeTimestamp);
	}
	else{
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion
		Timestamp *writeAddress = (Timestamp *)(tableIndex * sizeof(TPCC::StockVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion].writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateStockPointers(size_t offsetInLocalRegion, StockVersion *oldHead, uint16_t wID, RDMARegion<Timestamp> &localRegion, bool signaled) {
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<Timestamp> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableTimestampList;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();
	size_t offset = (size_t) offsetInLocalRegion * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[offset + i + 1].copy(versionArray[offset + i]);

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[offset + 0].copy(oldHead->writeTimestamp);

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM);
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], &localRegion.getRegion()[offset], size);
	}
	else{
		// the data sits at a remote server
		Timestamp *writeAddress =  (Timestamp *)(tableIndex * sizeof(Timestamp) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)oldHead->stock.S_I_ID << ", olNumber = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&versionArray[offset],
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateStockOlderVersions(size_t offsetInLocalRegion, StockVersion *oldHead, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, bool signaled) {
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableOlderVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	StockVersion *localBuffer = &localRegion.getRegion()[offsetInLocalRegion];
	memcpy(localBuffer, oldHead, sizeof(TPCC::StockVersion));

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM);
	size_t circularBufferIndex = (size_t) versionOffset;
	size_t index = tableIndex + circularBufferIndex;

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(StockVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[index], localBuffer, size);
	}
	else{
		// the data sits at a remote server
		StockVersion *writeAddress =  (StockVersion *)(index * sizeof(StockVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			std::cout << "timestamp: " << oldHead->writeTimestamp << std::endl;
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)oldHead->stock.S_I_ID
					<< ", versionOffset: " << (int)versionOffset << ", tableIndex: " << (int)tableIndex << ", circularBufferIndex: " << (int)circularBufferIndex);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localBuffer,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT) + clientRegionOffset);	// offset of OrderVersion in Order Table

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::OrderVersion *lookupAddress =  (TPCC::OrderVersion *)(tableIndex * sizeof(TPCC::OrderVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::insertIntoOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to which the order will be written
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset);	// offset of OrderVersion in OrderTable

	// Size to be written to the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else {
		// the data sits at a remote server
		TPCC::OrderVersion *writeAddress =  (TPCC::OrderVersion *)(tableIndex * sizeof(TPCC::OrderVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockOrder(TPCC::OrderVersion &orderV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset)  * sizeof(TPCC::OrderVersion));	// offset of OrderVersion in OrderTable
	size_t timestampOffset = (size_t)TPCC::OrderVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
		exit(-1);
	}

	uint64_t* oldTS_ULL = (uint64_t *) &orderV.writeTimestamp;
	uint64_t* newTS_ULL = (uint64_t *) &newTS;

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_ULL,
			*newTS_ULL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertOrderLock(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to revert the order timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset);	// offset of OrderVersion in OrderTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()->writeTimestamp);
	}
	else {
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::OrderVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderVersion
		uint64_t *writeAddress = (uint64_t *)(tableIndex * sizeof(TPCC::OrderVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()->writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}


void DBExecutor::retrieveNewOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::NewOrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT) + clientRegionOffset);	// offset of NewOrderVersion in NewOrder Table

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::NewOrderVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(localRegion.getRegion(), &remoteMH.region_[tableIndex], size);
	}
	else {
		// the data sits at a remote server
		TPCC::NewOrderVersion *lookupAddress =  (TPCC::NewOrderVersion *)(tableIndex * sizeof(TPCC::NewOrderVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::insertIntoNewOrder(primitive::client_id_t clientID, uint64_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::NewOrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to which the order will be written
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset);	// offset of NewOrderVersion in NewOrderTable

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::NewOrderVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else {
		// the data sits at a remote server
		TPCC::NewOrderVersion *writeAddress =  (TPCC::NewOrderVersion *)(tableIndex * sizeof(TPCC::NewOrderVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockNewOrder(TPCC::NewOrderVersion &orderV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::NewOrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset)  * sizeof(TPCC::NewOrderVersion));	// offset of NewOrderVersion in NewOrderTable
	size_t timestampOffset = (size_t)TPCC::NewOrderVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
		exit(-1);
	}

	uint64_t* oldTS_ULL = (uint64_t *) &orderV.writeTimestamp;
	uint64_t* newTS_ULL = (uint64_t *) &newTS;

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)localRegion.getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_ULL,
			*newTS_ULL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertNewOrderLock(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::NewOrderVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->newOrderTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to revert the NewOrder timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT + clientRegionOffset);	// offset of OrderVersion in OrderTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()->writeTimestamp);
	}
	else{
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::NewOrderVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderVersion
		uint64_t *writeAddress = (uint64_t *)(tableIndex * sizeof(TPCC::NewOrderVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", clientRegionOffset = " << (int)clientRegionOffset);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()->writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::updateStock(size_t offsetInLocalRegion, TPCC::StockVersion *stockV, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::StockVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->stockTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)(warehouseOffset * config::tpcc_settings::ITEMS_CNT + stockV->stock.S_I_ID);		// offset of StockVersion in StockTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], &localRegion.getRegion()[offsetInLocalRegion], size);
	}
	else{
		// the data sits at a remote server
		TPCC::StockVersion *writeAddress =  (TPCC::StockVersion *)(tableIndex * sizeof(TPCC::StockVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)stockV->stock.S_I_ID << ", olNumber = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::insertIntoOrderLine(primitive::client_id_t clientID, uint64_t clientRegionOffset, uint8_t offsetInLocalRegion, uint16_t wID,  RDMARegion<TPCC::OrderLineVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderLineVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + clientRegionOffset);	// offset of OLVersion in OLTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderLineVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], &localRegion.getRegion()[offsetInLocalRegion], size);
	}
	else {
		// the data sits at a remote server
		TPCC::OrderLineVersion *writeAddress =  (TPCC::OrderLineVersion *)(tableIndex * sizeof(TPCC::OrderLineVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", offsetInLocalRegion = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::retrieveOrderLines(primitive::client_id_t clientID, uint16_t wID, size_t clientRegionOffset, uint8_t numOfOrderlines,  RDMARegion<TPCC::OrderLineVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderLineVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to read the item info
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + clientRegionOffset);	// offset of OLVersion in OLTable

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (sizeof(TPCC::OrderLineVersion) * numOfOrderlines);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else {
		// the data sits at a remote server
		TPCC::OrderLineVersion *lookupAddress =  (TPCC::OrderLineVersion *)(tableIndex  * sizeof(TPCC::OrderLineVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);
		if (remoteMH.isAddressInRange((uintptr_t)lookupAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", #orderlines = " << (int)numOfOrderlines);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::lockOrderLine(size_t offsetInLocalRegion, TPCC::OrderLineVersion &orderLineV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderLineVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address of the timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( ( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + clientRegionOffset)  * sizeof(TPCC::OrderLineVersion));	// offset of OLVersion in OLTable
	size_t timestampOffset = (size_t)TPCC::OrderLineVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderLineVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", offsetInLocalRegion = " << (int)offsetInLocalRegion);
		exit(-1);
	}

	uint64_t* oldTS_ULL = (uint64_t *) &orderLineV.writeTimestamp;
	uint64_t *newTS_ULL = (uint64_t *)&newTS;

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_ULL,
			*newTS_ULL,
			signaled));
	if (signaled) outstandingSendCompletionCnt_++;
}

void DBExecutor::revertOrderLineLock(size_t offsetInLocalRegion, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderLineVersion> &localRegion, bool signaled){
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::OrderLineVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->orderLineTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	// The remote address to revert the NewOrder timestamp
	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + clientRegionOffset);	// offset of OLVersion in OLTable

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		remoteMH.region_[tableIndex].writeTimestamp.copy(localRegion.getRegion()[offsetInLocalRegion].writeTimestamp);
	}
	else {
		// the data sits at a remote server
		size_t timestampOffset = (size_t)TPCC::OrderLineVersion::getOffsetOfTimestamp();		// offset of Timestamp in OrderLineVersion
		uint64_t *writeAddress = (uint64_t *)(tableIndex * sizeof(TPCC::OrderLineVersion) + timestampOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: ClientID: " << (int)clientID << ", WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", offsetInLocalRegion = " << (int)offsetInLocalRegion);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)&localRegion.getRegion()[offsetInLocalRegion].writeTimestamp,
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}

void DBExecutor::insertIntoHistory(primitive::client_id_t clientID, uint64_t hID, RDMARegion<TPCC::HistoryVersion> &localRegion, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->history.H_W_ID;
	size_t serverNum = Warehouse::getServerNum(wID);
	MemoryHandler<TPCC::HistoryVersion> &remoteMH = dsCtx_[serverNum]->getRemoteMemoryKeys()->getRegion()->historyTableHeadVersions;
	ibv_qp *qp = dsCtx_[serverNum]->getQP();

	uint16_t warehouseOffset = Warehouse::getWarehouseOffsetOnServer(wID);
	size_t tableIndex = (size_t)( (clientID * config::tpcc_settings::HISTORY_BUFFER_PER_CLIENT) + hID);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::HistoryVersion);

	if (isServerLocal(serverNum)) {
		// the data sits at a co-located server
		std::memcpy(&remoteMH.region_[tableIndex], localRegion.getRegion(), size);
	}
	else {
		// the data sits at a remote server
		TPCC::HistoryVersion *writeAddress = (TPCC::HistoryVersion *)(tableIndex * sizeof(TPCC::HistoryVersion) + (uint64_t)remoteMH.rdmaHandler_.addr);

		if (remoteMH.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", hID = " << (int)hID);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				qp,
				localRegion.getRDMAHandler(),
				(uintptr_t)localRegion.getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)writeAddress,
				size,
				signaled));
		if (signaled) outstandingSendCompletionCnt_++;
	}
}


} /* namespace TPCC */
