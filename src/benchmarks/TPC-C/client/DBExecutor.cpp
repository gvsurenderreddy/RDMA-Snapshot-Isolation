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
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
}

DBExecutor::DBExecutor() {

}

uint16_t DBExecutor::getWarehouseOffsetOnServer(uint16_t wID){
	size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	return (uint16_t)(wID - (serverNum * config::tpcc_settings::WAREHOUSE_PER_SERVER));
}

template <typename T>
bool DBExecutor::isAddressInRange(uintptr_t lookupAddress, MemoryHandler<T> remoteMH) {
	if ((lookupAddress < (uintptr_t) remoteMH.rdmaHandler_.addr) || (lookupAddress >= (uintptr_t)remoteMH.rdmaHandler_.addr + remoteMH.regionSizeInBytes_)){
		PRINT_CERR(CLASS_NAME, __func__, "Accessing outside the region: " << lookupAddress << " NOT IN ["
				<< (uintptr_t) remoteMH.rdmaHandler_.addr << ", " << (uintptr_t)remoteMH.rdmaHandler_.addr + remoteMH.regionSizeInBytes_ << ") range. Region size: " << (int)remoteMH.regionSize_);
		return false;
	}
	else return true;
}

void DBExecutor::lookupCustomerByLastName(primitive::client_id_t clientID, uint16_t wID, uint8_t dID, const char *cLastName, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::IndexResponseMessage> &responseRegion, ibv_qp *qp, bool signaled){
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

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
			sizeof(IndexResponseMessage)));

	TEST_NZ (RDMACommon::post_SEND(
			qp,
			requestRegion.getRDMAHandler(),
			(uintptr_t)requestRegion.getRegion(),
			sizeof(IndexRequestMessage),
			signaled));
}

void DBExecutor::getReadTimestamp(RDMARegion<primitive::timestamp_t> &localRegion, MemoryHandler<primitive::timestamp_t> &remoteMH, ibv_qp *qp) {
	primitive::timestamp_t *lookupAddress = (primitive::timestamp_t*)remoteMH.rdmaHandler_.addr;
	uint32_t size = (uint32_t)(remoteMH.regionSize_ * sizeof(primitive::timestamp_t));

	if(isAddressInRange<primitive::timestamp_t>((uintptr_t)lookupAddress, remoteMH) == false){
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
			false));
}

void DBExecutor::submitResult(primitive::client_id_t clientID, RDMARegion<primitive::timestamp_t> &localRegion,  MemoryHandler<primitive::timestamp_t> &remoteMH, ibv_qp *qp){
	// The remote address of the timestamp
	size_t tableOffset = (size_t)(clientID * sizeof(primitive::timestamp_t));		// offset of client's cts in timestampVector
	primitive::timestamp_t *writeAddress = (primitive::timestamp_t*)(tableOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	uint32_t size = (uint32_t) sizeof(primitive::timestamp_t);

	if (isAddressInRange<primitive::timestamp_t>((uintptr_t)writeAddress, remoteMH) == false){
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
			true));
}

void DBExecutor::retrieveWarehouse(uint16_t wID, RDMARegion<WarehouseVersion> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	TPCC::WarehouseVersion *lookupAddress =  (WarehouseVersion*)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(WarehouseVersion);

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::retrieveWarehouseTax(uint16_t wID, RDMARegion<TPCC::WarehouseVersion> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp){
	// The remote address to read the warehouse tax
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	size_t payloadOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfWarehouse();		// offset of Warehouse in WarehouseVersion
	size_t fieldOffset = TPCC::Warehouse::getOffsetOfTax();								// offset of W_TAX in Warehouse
	float *lookupAddress =  (float*)(tableOffset + payloadOffset + fieldOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// Warehouse::W_TAX is of type float

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)lookupAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID );
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&(localRegion.getRegion()->warehouse.W_TAX),
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));
}

void DBExecutor::retrieveWarehousePointerList(uint16_t wID, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *lookupAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::lockWarehouse(TPCC::WarehouseVersion &warehouseV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp){
	// The remote address of the timestamp
	uint16_t wID = warehouseV.warehouse.W_ID;
	uint64_t* oldTS_UUL = (uint64_t *) &warehouseV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	size_t timestampOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfTimestamp();		// offset of Timestamp in WarehouseVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
			*newTS_UUL));
}

void DBExecutor::revertWarehouseLock(RDMARegion<TPCC::WarehouseVersion> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp, bool signaled){
	uint16_t wID = localRegion.getRegion()->warehouse.W_ID;

	// The remote address of the timestamp

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));	// offset of WarehouseVersion in WarehouseTable
	size_t timestampOffset = (size_t)TPCC::WarehouseVersion::getOffsetOfTimestamp();	// offset of Timestamp in WarehouseVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateWarehousePointers(TPCC::WarehouseVersion &oldHead, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1] = versionArray[i];

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0] = oldHead.writeTimestamp;

	// The remote address to read the item info
	uint16_t wID = oldHead.warehouse.W_ID;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *writeAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateWarehouseOlderVersions(RDMARegion<TPCC::WarehouseVersion> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp, bool signaled) {
	TPCC::WarehouseVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->warehouse.W_ID;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * config::tpcc_settings::VERSION_NUM * sizeof(TPCC::WarehouseVersion));
	size_t circularBufferOffset = (size_t) versionOffset * sizeof(TPCC::WarehouseVersion);
	WarehouseVersion *writeAddress =  (WarehouseVersion *)(tableOffset + circularBufferOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::WarehouseVersion);

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateWarehouse(RDMARegion<TPCC::WarehouseVersion> &localRegion, MemoryHandler<TPCC::WarehouseVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->warehouse.W_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(warehouseOffset * sizeof(TPCC::WarehouseVersion));
	TPCC::WarehouseVersion *writeAddress =  (TPCC::WarehouseVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::WarehouseVersion);

	if (isAddressInRange<TPCC::WarehouseVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}














void DBExecutor::retrieveDistrict(uint16_t wID, uint8_t dID, RDMARegion<DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp* qp, bool signaled){
	// The remote address to read the district tax
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	TPCC::DistrictVersion* lookupAddress =  (TPCC::DistrictVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::retrieveDistrictTax(uint16_t wID, uint8_t dID, RDMARegion<DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp* qp){
	// The remote address to read the district tax
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfTax();		// offset of D_TAX in District
	float *lookupAddress =  (float *)(tableOffset + districtOffset + fieldOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(float);	// District::D_TAX is of type float

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)lookupAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", dID = " << (int)dID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&(localRegion.getRegion()->district.D_TAX),
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			false));
}

void DBExecutor::retrieveDistrictPointerList(uint16_t wID, uint8_t dID, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *lookupAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID, RDMARegion<TPCC::DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp *qp){
	// The remote address to read the district tax
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t districtOffset = (size_t)TPCC::DistrictVersion::getOffsetOfDistrict();		// offset of District in DistrictVersion
	size_t fieldOffset = TPCC::District::getOffsetOfNextOID();		// offset of D_NEXT_O_ID in District
	uint64_t *lookupAddress =  (uint64_t *)(tableOffset + districtOffset + fieldOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	size_t size = sizeof(localRegion.getRegion()->district.D_NEXT_O_ID);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)lookupAddress, remoteMH) == false){
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
			(uint32_t)size));
}

void DBExecutor::lockDistrict(TPCC::DistrictVersion &districtV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp *qp){
	// The remote address of the timestamp
	uint16_t wID = districtV.district.D_W_ID;
	uint8_t dID = districtV.district.D_ID;

	uint64_t* oldTS_UUL = (uint64_t *) &districtV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t timestampOffset = (size_t)TPCC::DistrictVersion::getOffsetOfTimestamp();		// offset of Timestamp in DistrictVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
			*newTS_UUL));
}

void DBExecutor::revertDistrictLock(RDMARegion<TPCC::DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp *qp, bool signaled){
	uint16_t wID = localRegion.getRegion()->district.D_W_ID;
	uint8_t dID = localRegion.getRegion()->district.D_ID;

	// The remote address of the timestamp
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(localRegion.getRegion()->district.D_W_ID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t timestampOffset = (size_t)TPCC::DistrictVersion::getOffsetOfTimestamp();	// offset of Timestamp in DistrictVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateDistrictPointers(TPCC::DistrictVersion &oldHead, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1] = versionArray[i];

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0] = oldHead.writeTimestamp;

	// The remote address to read the item info
	uint16_t wID = oldHead.district.D_W_ID;
	uint8_t dID = oldHead.district.D_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));	// offset of DistrictVersion in DistrictTable
	Timestamp *writeAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateDistrictOlderVersions(RDMARegion<TPCC::DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp *qp, bool signaled) {
	TPCC::DistrictVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->district.D_W_ID;
	uint8_t dID = oldHead->district.D_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * config::tpcc_settings::VERSION_NUM * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	size_t circularBufferOffset = (size_t) versionOffset * sizeof(TPCC::DistrictVersion);
	DistrictVersion *writeAddress =  (DistrictVersion *)(tableOffset + circularBufferOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateDistrict(RDMARegion<TPCC::DistrictVersion> &localRegion, MemoryHandler<TPCC::DistrictVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->district.D_W_ID;
	uint8_t dID = localRegion.getRegion()->district.D_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID) * sizeof(TPCC::DistrictVersion));	// offset of DistrictVersion in DistrictTable
	TPCC::DistrictVersion *writeAddress =  (TPCC::DistrictVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::DistrictVersion);

	if (isAddressInRange<TPCC::DistrictVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}




void DBExecutor::retrieveCustomer(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<CustomerVersion> &localRegion, MemoryHandler<TPCC::CustomerVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the customer info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* sizeof(TPCC::CustomerVersion));										// offset of CustomerVersion in CustomerTable
	TPCC::CustomerVersion *lookupAddress =  (TPCC::CustomerVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isAddressInRange<TPCC::CustomerVersion>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::retrieveCustomerPointerList(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)
			* config::tpcc_settings::VERSION_NUM  * sizeof(Timestamp));

	Timestamp *lookupAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)lookupAddress, remoteMH) == false){
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
}

void DBExecutor::lockCustomer(TPCC::CustomerVersion &customerV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, MemoryHandler<TPCC::CustomerVersion> &remoteMH, ibv_qp *qp){
	// The remote address of the timestamp
	uint16_t wID = customerV.customer.C_W_ID;
	uint8_t dID = customerV.customer.C_D_ID;
	uint32_t cID = customerV.customer.C_ID;

	uint64_t* oldTS_UUL = (uint64_t *) &customerV.writeTimestamp;
	uint64_t* newTS_UUL = (uint64_t *) &newTS;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);

	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * sizeof(TPCC::CustomerVersion));
	size_t timestampOffset = (size_t)TPCC::CustomerVersion::getOffsetOfTimestamp();		// offset of Timestamp in CustomerVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (isAddressInRange<TPCC::CustomerVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
			*newTS_UUL));
}

void DBExecutor::revertCustomerLock(RDMARegion<TPCC::CustomerVersion> &localRegion, MemoryHandler<TPCC::CustomerVersion> &remoteMH, ibv_qp *qp, bool signaled){
	uint16_t wID = localRegion.getRegion()->customer.C_W_ID;
	uint8_t dID = localRegion.getRegion()->customer.C_D_ID;
	uint32_t cID = localRegion.getRegion()->customer.C_ID;

	// The remote address of the timestamp
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * sizeof(TPCC::CustomerVersion));	// offset of CustomerVersion in CustomerTable
	size_t timestampOffset = (size_t)TPCC::CustomerVersion::getOffsetOfTimestamp();	// offset of Timestamp in CustomerVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isAddressInRange<TPCC::CustomerVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateCustomerPointers(TPCC::CustomerVersion &oldHead, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[i + 1] = versionArray[i];

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[0] = oldHead.writeTimestamp;

	// The remote address to read the item info
	uint16_t wID = oldHead.customer.C_W_ID;
	uint8_t dID = oldHead.customer.C_D_ID;
	uint32_t cID = oldHead.customer.C_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *writeAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateCustomerOlderVersions(RDMARegion<TPCC::CustomerVersion> &localRegion, MemoryHandler<TPCC::CustomerVersion> &remoteMH, ibv_qp *qp, bool signaled) {
	TPCC::CustomerVersion *oldHead = localRegion.getRegion();
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	// The remote address to read the item info
	uint16_t wID = oldHead->customer.C_W_ID;
	uint8_t dID = oldHead->customer.C_D_ID;
	uint32_t cID = oldHead->customer.C_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID) * config::tpcc_settings::VERSION_NUM * sizeof(TPCC::CustomerVersion));
	size_t circularBufferOffset = (size_t) versionOffset * sizeof(TPCC::CustomerVersion);
	CustomerVersion *writeAddress =  (CustomerVersion *)(tableOffset + circularBufferOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isAddressInRange<TPCC::CustomerVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}

void DBExecutor::updateCustomer(RDMARegion<TPCC::CustomerVersion> &localRegion, MemoryHandler<TPCC::CustomerVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->customer.C_W_ID;
	uint8_t dID = localRegion.getRegion()->customer.C_D_ID;
	uint32_t cID = localRegion.getRegion()->customer.C_ID;

	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)(((warehouseOffset * config::tpcc_settings::DISTRICT_PER_WAREHOUSE + dID)
			* config::tpcc_settings::CUSTOMER_PER_DISTRICT + cID)  * sizeof(TPCC::CustomerVersion));

	TPCC::CustomerVersion *writeAddress =  (TPCC::CustomerVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::CustomerVersion);

	if (isAddressInRange<TPCC::CustomerVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}




void DBExecutor::retrieveItem(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<ItemVersion> &localRegion, MemoryHandler<TPCC::ItemVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	size_t tableOffset = (size_t)(iID * sizeof(TPCC::ItemVersion));		// offset of ItemVersion in ItemTable
	TPCC::ItemVersion *lookupAddress =  (TPCC::ItemVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::ItemVersion);

	if (isAddressInRange<TPCC::ItemVersion>((uintptr_t)lookupAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: wID = " << (int)wID << ", iID = " << (int)iID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber],
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			signaled));
}

void DBExecutor::retrieveStock(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<StockVersion> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	TPCC::StockVersion *lookupAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	if (isAddressInRange<TPCC::StockVersion>((uintptr_t)lookupAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber],
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			signaled));
}

void DBExecutor::retrieveStockPointerList(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *lookupAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock

	if (isAddressInRange<Timestamp>((uintptr_t)lookupAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)olNumber);
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
}

void DBExecutor::lockStock(uint8_t olNumber, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp){
	// The remote address of the timestamp
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion
	uint64_t *writeAddress = (uint64_t *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(uint64_t);

	if (isAddressInRange<TPCC::StockVersion>((uintptr_t)writeAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)olNumber);
		exit(-1);
	}

	uint64_t *oldTS_ULL = (uint64_t *)&oldTS;
	uint64_t *newTS_ULL = (uint64_t *)&newTS;

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			*oldTS_ULL,
			*newTS_ULL));
}

void DBExecutor::revertStockLock(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address of the timestamp
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + iID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	size_t timestampOffset = (size_t)TPCC::StockVersion::getOffsetOfTimestamp();		// offset of Timestamp in StockVersion
	Timestamp *writeAddress = (Timestamp *)(tableOffset + timestampOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	uint32_t size = (uint32_t) sizeof(Timestamp);

	if (isAddressInRange<TPCC::StockVersion>((uintptr_t)writeAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)iID << ", olNumber = " << (int)olNumber);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
			IBV_WR_RDMA_WRITE,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber].writeTimestamp,
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			signaled));
}

void DBExecutor::updateStockPointers(uint8_t olNumber, StockVersion *oldHead, uint16_t wID, RDMARegion<Timestamp> &localRegion, MemoryHandler<Timestamp> &remoteMH, ibv_qp *qp, bool signaled) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	Timestamp *versionArray = localRegion.getRegion();
	size_t offset = (size_t) olNumber * config::tpcc_settings::VERSION_NUM;		// offset of versions for the given stock

	for (int i = config::tpcc_settings::VERSION_NUM - 2; i >= 0; i--)
		versionArray[offset + i + 1] = versionArray[offset + i];

	// second, set the head of the pointer list to point to the head of the old versions
	versionArray[offset + 0] = oldHead->writeTimestamp;

	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));
	Timestamp *writeAddress =  (Timestamp *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) (config::tpcc_settings::VERSION_NUM * sizeof(Timestamp));

	if (isAddressInRange<Timestamp>((uintptr_t)writeAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)oldHead->stock.S_I_ID << ", olNumber = " << (int)olNumber);
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
}

void DBExecutor::updateStockOlderVersions(uint8_t olNumber, StockVersion *oldHead, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp, bool signaled) {
	primitive::version_offset_t versionOffset = oldHead->writeTimestamp.getVersionOffset();

	StockVersion *localBuffer = &localRegion.getRegion()[olNumber];
	memcpy(localBuffer, oldHead, sizeof(TPCC::StockVersion));

	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + oldHead->stock.S_I_ID) * config::tpcc_settings::VERSION_NUM * sizeof(StockVersion));
	size_t circularBufferOffset = (size_t) versionOffset * sizeof(StockVersion);
	StockVersion *writeAddress =  (StockVersion *)(tableOffset + circularBufferOffset + (uint64_t)remoteMH.rdmaHandler_.addr);

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(StockVersion);

	if (isAddressInRange<TPCC::StockVersion>((uintptr_t)writeAddress, remoteMH) == false){
		std::cout << "timestamp: " << oldHead->writeTimestamp << std::endl;
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)oldHead->stock.S_I_ID
				<< ", versionOffset: " << (int)versionOffset << ", tableOffset: " << (int)tableOffset << ", circularBufferOffset: " << (int)circularBufferOffset);
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
}

void DBExecutor::insertIntoOrder(primitive::client_id_t clientID, uint64_t nextOrderID, uint16_t wID, RDMARegion<TPCC::OrderVersion> &localRegion, MemoryHandler<TPCC::OrderVersion> &remoteMH, ibv_qp *qp){
	// The remote address to which the order will be written
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( (clientID * config::tpcc_settings::ORDER_PER_CLIENT + nextOrderID)  * sizeof(TPCC::OrderVersion));	// offset of OrderVersion in OrderTable
	TPCC::OrderVersion *writeAddress =  (TPCC::OrderVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderVersion);

	if (isAddressInRange<TPCC::OrderVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
			false));
}

void DBExecutor::insertIntoNewOrder(primitive::client_id_t clientID, uint64_t nextNewOrderID, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &localRegion, MemoryHandler<TPCC::NewOrderVersion> &remoteMH, ibv_qp *qp){
	// The remote address to which the order will be written
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( (clientID * config::tpcc_settings::ORDER_PER_CLIENT + nextNewOrderID)  * sizeof(TPCC::NewOrderVersion));	// offset of NewOrderVersion in NewOrderTable
	TPCC::NewOrderVersion *writeAddress =  (TPCC::NewOrderVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be written tothe remote side
	uint32_t size = (uint32_t) sizeof(TPCC::NewOrderVersion);

	if (isAddressInRange<TPCC::NewOrderVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
			false));
}

void DBExecutor::updateStock(uint8_t olNumber, TPCC::StockVersion *stockV, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)((warehouseOffset * config::tpcc_settings::ITEMS_CNT + stockV->stock.S_I_ID) * sizeof(TPCC::StockVersion));		// offset of StockVersion in StockTable
	TPCC::StockVersion *writeAddress =  (TPCC::StockVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::StockVersion);

	if (isAddressInRange<TPCC::StockVersion>((uintptr_t)writeAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: warehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID << ", iID = " << (int)stockV->stock.S_I_ID << ", olNumber = " << (int)olNumber);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			false));
}

void DBExecutor::insertIntoOrderLine(primitive::client_id_t clientID, uint64_t olID, uint8_t olNumber, uint16_t wID,  RDMARegion<TPCC::OrderLineVersion> &localRegion, MemoryHandler<TPCC::OrderLineVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( ( (clientID * config::tpcc_settings::ORDER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) + olID)  * sizeof(TPCC::OrderLineVersion));	// offset of OLVersion in OLTable
	TPCC::OrderLineVersion *writeAddress =  (TPCC::OrderLineVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::OrderLineVersion);

	if (isAddressInRange<TPCC::OrderLineVersion>((uintptr_t)writeAddress, remoteMH) == false){
		PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: WarehouseOffset = " << (int)warehouseOffset << ", wID = " << (int)wID  << ", olNumber = " << (int)olNumber);
		exit(-1);
	}

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			qp,
			localRegion.getRDMAHandler(),
			(uintptr_t)&localRegion.getRegion()[olNumber],
			&remoteMH.rdmaHandler_,
			(uintptr_t)writeAddress,
			size,
			signaled));
}

void DBExecutor::insertIntoHistory(primitive::client_id_t clientID, uint64_t hID, RDMARegion<TPCC::HistoryVersion> &localRegion, MemoryHandler<TPCC::HistoryVersion> &remoteMH, ibv_qp *qp, bool signaled){
	// The remote address to read the item info
	uint16_t wID = localRegion.getRegion()->history.H_W_ID;
	uint16_t warehouseOffset = getWarehouseOffsetOnServer(wID);
	size_t tableOffset = (size_t)( ( (clientID * config::tpcc_settings::HISTORY_PER_CLIENT) + hID)  * sizeof(TPCC::HistoryVersion));
	TPCC::HistoryVersion *writeAddress = (TPCC::HistoryVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

	// Size to be read from the remote side
	uint32_t size = (uint32_t) sizeof(TPCC::HistoryVersion);

	if (isAddressInRange<TPCC::HistoryVersion>((uintptr_t)writeAddress, remoteMH) == false){
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
}


} /* namespace TPCC */
