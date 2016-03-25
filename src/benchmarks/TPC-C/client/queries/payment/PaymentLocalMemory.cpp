/*
 * PaymentLocalMemory.cpp
 *
 *  Created on: Mar 15, 2016
 *      Author: erfanz
 */

#include "PaymentLocalMemory.hpp"
#include "../../../../../util/utils.hpp"
#include "../../../tables/TPCCUtil.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"PaymentLocMem"

namespace TPCC{
PaymentLocalMemory::PaymentLocalMemory(RDMAContext &context){
	customerHead_			= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	customerOlderVersions_ 	= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	districtHead_			= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	districtTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	districtOlderVersions_ 	= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	historyHead_			= new RDMARegion<TPCC::HistoryVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	historyTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	historyOlderVersions_ 	= new RDMARegion<TPCC::HistoryVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	warehouseHead_			= new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	warehouseTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	warehouseOlderVersions_ = new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	warehouseLockRegion_	= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	districtLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);

}

RDMARegion<TPCC::CustomerVersion>* PaymentLocalMemory::getCustomerHead() {
	return customerHead_;
}

RDMARegion<Timestamp>* PaymentLocalMemory::getCustomerTS() {
	return customerTS_;
}

RDMARegion<TPCC::CustomerVersion>* PaymentLocalMemory::getCustomerOlderVersions() {
	return customerOlderVersions_;
}

RDMARegion<TPCC::DistrictVersion>* PaymentLocalMemory::getDistrictHead(){
	return districtHead_;
}
RDMARegion<Timestamp>* PaymentLocalMemory::getDistrictTS(){
	return districtTS_;
}
RDMARegion<TPCC::DistrictVersion>* PaymentLocalMemory::getDistrictOlderVersions(){
	return districtOlderVersions_;
}

RDMARegion<TPCC::HistoryVersion>* PaymentLocalMemory::getHistoryHead(){
	return historyHead_;
}
RDMARegion<Timestamp>* PaymentLocalMemory::getHistoryTS(){
	return historyTS_;
}
RDMARegion<TPCC::HistoryVersion>* PaymentLocalMemory::getHistoryOlderVersions(){
	return historyOlderVersions_;
}

RDMARegion<TPCC::WarehouseVersion>* PaymentLocalMemory::getWarehouseHead(){
	return warehouseHead_;
}
RDMARegion<Timestamp>* PaymentLocalMemory::getWarehouseTS(){
	return warehouseTS_;
}
RDMARegion<TPCC::WarehouseVersion>* PaymentLocalMemory::getWarehouseOlderVersions(){
	return warehouseOlderVersions_;
}

RDMARegion<uint64_t>* PaymentLocalMemory::getWarehouseLockRegion(){
	return warehouseLockRegion_;
}

RDMARegion<uint64_t>* PaymentLocalMemory::getDistrictLockRegion(){
	return districtLockRegion_;
}

RDMARegion<uint64_t>* PaymentLocalMemory::getCustomerLockRegion(){
	return customerLockRegion_;
}

PaymentLocalMemory::~PaymentLocalMemory(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");

	delete customerHead_;
	delete customerTS_;
	delete customerOlderVersions_;

	delete districtHead_;
	delete districtTS_;
	delete districtOlderVersions_;

	delete historyHead_;
	delete historyTS_;
	delete historyOlderVersions_;

	delete warehouseHead_;
	delete warehouseTS_;
	delete warehouseOlderVersions_;

	delete warehouseLockRegion_;
	delete districtLockRegion_;
	delete customerLockRegion_;
}
}
