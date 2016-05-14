/*
 * OrderStatusLocalMemory.cpp
 *
 *  Created on: Apr 4, 2016
 *      Author: erfanz
 */

#include "OrderStatusLocalMemory.hpp"
#include "../../../../../../util/utils.hpp"
#include "../../../../tables/TPCCUtil.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"OrdStatLocMem"

namespace TPCC {
OrderStatusLocalMemory::OrderStatusLocalMemory(std::ostream &os, RDMAContext &context)
: os_(os){

	orderHead_				= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	orderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderOlderVersions_ 	= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	orderLineHead_			= new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineTS_			= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineOlderVersions_ = new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
}

OrderStatusLocalMemory::~OrderStatusLocalMemory(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	delete orderHead_;
	delete orderTS_;
	delete orderOlderVersions_;

	delete orderLineHead_;
	delete orderLineTS_;
	delete orderLineOlderVersions_;
}

RDMARegion<TPCC::OrderVersion>* OrderStatusLocalMemory::getOrderHead(){
	return orderHead_;
}

RDMARegion<Timestamp>* OrderStatusLocalMemory::getOrderTS(){
	return orderTS_;
}

RDMARegion<TPCC::OrderVersion>* OrderStatusLocalMemory::getOrderOlderVersions(){
	return orderOlderVersions_;
}

RDMARegion<TPCC::OrderLineVersion>* OrderStatusLocalMemory::getOrderLineHead(){
	return orderLineHead_;
}

RDMARegion<Timestamp>* OrderStatusLocalMemory::getOrderLineTS(){
	return orderLineTS_;
}

RDMARegion<TPCC::OrderLineVersion>* OrderStatusLocalMemory::getOrderLineOlderVersions(){
	return orderLineOlderVersions_;
}

} /* namespace TPCC */
