/*
 * StockLevelLocalMemory.cpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#include "StockLevelLocalMemory.hpp"
#include "../../../../../../util/utils.hpp"
#include "../../../../tables/TPCCUtil.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"StockLvlLocMem"

namespace TPCC {
StockLevelLocalMemory::StockLevelLocalMemory(std::ostream &os, RDMAContext &context)
: os_(os){

	districtHead_			= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	districtTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	districtOlderVersions_ 	= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

//	orderHead_				= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
//	orderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
//	orderOlderVersions_ 	= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
//
//	orderLineHead_			= new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
//	orderLineTS_			= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
//	orderLineOlderVersions_ = new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);

	stockHead_				= new RDMARegion<TPCC::StockVersion>(20 * tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	stockTS_				= new RDMARegion<Timestamp>(20 * tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	stockOlderVersions_ 	= new RDMARegion<TPCC::StockVersion>(20 * tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
}

StockLevelLocalMemory::~StockLevelLocalMemory(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	delete districtHead_;
	delete districtTS_;
	delete districtOlderVersions_;

//	delete orderHead_;
//	delete orderTS_;
//	delete orderOlderVersions_;
//
//	delete orderLineHead_;
//	delete orderLineTS_;
//	delete orderLineOlderVersions_;

	delete stockHead_;
	delete stockTS_;
	delete stockOlderVersions_;
}

RDMARegion<TPCC::DistrictVersion>* StockLevelLocalMemory::getDistrictHead(){
	return districtHead_;
}

RDMARegion<Timestamp>* StockLevelLocalMemory::getDistrictTS(){
	return districtTS_;
}

RDMARegion<TPCC::DistrictVersion>* StockLevelLocalMemory::getDistrictOlderVersions(){
	return districtOlderVersions_;
}

//RDMARegion<TPCC::OrderVersion>* StockLevelLocalMemory::getOrderHead(){
//	return orderHead_;
//}
//
//RDMARegion<Timestamp>* StockLevelLocalMemory::getOrderTS(){
//	return orderTS_;
//}
//
//RDMARegion<TPCC::OrderVersion>* StockLevelLocalMemory::getOrderOlderVersions(){
//	return orderOlderVersions_;
//}
//
//RDMARegion<TPCC::OrderLineVersion>* StockLevelLocalMemory::getOrderLineHead(){
//	return orderLineHead_;
//}
//
//RDMARegion<Timestamp>* StockLevelLocalMemory::getOrderLineTS(){
//	return orderLineTS_;
//}
//
//RDMARegion<TPCC::OrderLineVersion>* StockLevelLocalMemory::getOrderLineOlderVersions(){
//	return orderLineOlderVersions_;
//}

RDMARegion<TPCC::StockVersion>* StockLevelLocalMemory::getStockHead(){
	return stockHead_;
}

RDMARegion<Timestamp>* StockLevelLocalMemory::getStockTS(){
	return stockTS_;
}

RDMARegion<TPCC::StockVersion>* StockLevelLocalMemory::getStockOlderVersions(){
	return stockOlderVersions_;
}

} /* namespace TPCC */
