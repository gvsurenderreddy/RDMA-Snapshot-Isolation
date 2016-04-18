/*
 * DeliveryLocalMemory.cpp
 *
 *  Created on: Apr 14, 2016
 *      Author: erfanz
 */

#include "DeliveryLocalMemory.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"DeliveryLocMem"

namespace TPCC {

DeliveryLocalMemory::DeliveryLocalMemory(std::ostream &os, RDMAContext &context)
: os_(os){
	customerHead_			= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	customerOlderVersions_ 	= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	newOrderHead_			= new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	newOrderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	newOrderOlderVersions_ 	= new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	orderHead_				= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	orderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderOlderVersions_ 	= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	orderLineHead_			= new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineTS_			= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineOlderVersions_ = new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);

	customerLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	orderLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	newOrderLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	orderLinesLocksRegion_	= new RDMARegion<uint64_t>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
}

RDMARegion<TPCC::CustomerVersion>* DeliveryLocalMemory::getCustomerHead() {
	return customerHead_;
}

RDMARegion<Timestamp>* DeliveryLocalMemory::getCustomerTS() {
	return customerTS_;
}

RDMARegion<TPCC::CustomerVersion>* DeliveryLocalMemory::getCustomerOlderVersions() {
	return customerOlderVersions_;
}

RDMARegion<TPCC::NewOrderVersion>* DeliveryLocalMemory::getNewOrderHead(){
	return newOrderHead_;
}

RDMARegion<Timestamp>* DeliveryLocalMemory::getNewOrderTS(){
	return newOrderTS_;
}

RDMARegion<TPCC::NewOrderVersion>* DeliveryLocalMemory::getNewOrderOlderVersions(){
	return newOrderOlderVersions_;
}

RDMARegion<TPCC::OrderVersion>* DeliveryLocalMemory::getOrderHead(){
	return orderHead_;
}

RDMARegion<Timestamp>* DeliveryLocalMemory::getOrderTS(){
	return orderTS_;
}

RDMARegion<TPCC::OrderVersion>* DeliveryLocalMemory::getOrderOlderVersions(){
	return orderOlderVersions_;
}

RDMARegion<TPCC::OrderLineVersion>* DeliveryLocalMemory::getOrderLineHead(){
	return orderLineHead_;
}

RDMARegion<Timestamp>* DeliveryLocalMemory::getOrderLineTS(){
	return orderLineTS_;
}

RDMARegion<TPCC::OrderLineVersion>* DeliveryLocalMemory::getOrderLineOlderVersions(){
	return orderLineOlderVersions_;
}

RDMARegion<uint64_t>* DeliveryLocalMemory::getCustomerLockRegion(){
	return customerLockRegion_;
}

RDMARegion<uint64_t>* DeliveryLocalMemory::getNewOrderLockRegion(){
	return newOrderLockRegion_;
}

RDMARegion<uint64_t>* DeliveryLocalMemory::getOrderLockRegion(){
	return orderLockRegion_;
}

RDMARegion<uint64_t>* DeliveryLocalMemory::getOrderLinesLocksRegion(){
	return orderLinesLocksRegion_;
}

DeliveryLocalMemory::~DeliveryLocalMemory(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	delete customerHead_;
	delete customerTS_;
	delete customerOlderVersions_;

	delete orderHead_;
	delete orderTS_;
	delete orderOlderVersions_;

	delete newOrderHead_;
	delete newOrderTS_;
	delete newOrderOlderVersions_;

	delete orderLineHead_;
	delete orderLineTS_;
	delete orderLineOlderVersions_;

	delete customerLockRegion_;
	delete orderLockRegion_;
	delete newOrderLockRegion_;
	delete orderLinesLocksRegion_;
}

} /* namespace TPCC */
