/*
 * LocalMemory.cpp
 *
 *  Created on: Feb 26, 2016
 *      Author: erfanz
 */

#include "NewOrderLocalMemory.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"NewOrdLocMem"

TPCC::NewOrderLocalMemory::NewOrderLocalMemory(std::ostream &os, RDMAContext &context)
: os_(os){

	customerHead_			= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	customerOlderVersions_ 	= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	districtHead_			= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	districtTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	districtOlderVersions_ 	= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	itemHead_				= new RDMARegion<TPCC::ItemVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	itemTS_					= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	itemOlderVersions_ 		= new RDMARegion<TPCC::ItemVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);

	newOrderHead_			= new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	newOrderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	newOrderOlderVersions_ 	= new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	orderLineHead_			= new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineTS_			= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderLineOlderVersions_ = new RDMARegion<TPCC::OrderLineVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);

	orderHead_				= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	orderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	orderOlderVersions_ 	= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	stockHead_				= new RDMARegion<TPCC::StockVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
	stockTS_				= new RDMARegion<Timestamp>(tpcc_settings::ORDER_MAX_OL_CNT * config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	stockOlderVersions_ 	= new RDMARegion<TPCC::StockVersion>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);

	warehouseHead_			= new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	warehouseTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	warehouseOlderVersions_ = new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	districtLockRegion_		= new RDMARegion<uint64_t>(1, context, IBV_ACCESS_LOCAL_WRITE);
	stocksLocksRegion_		= new RDMARegion<uint64_t>(tpcc_settings::ORDER_MAX_OL_CNT, context, IBV_ACCESS_LOCAL_WRITE);
}

RDMARegion<TPCC::CustomerVersion>* TPCC::NewOrderLocalMemory::getCustomerHead() {
	return customerHead_;
}

RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getCustomerTS() {
	return customerTS_;
}

RDMARegion<TPCC::CustomerVersion>* TPCC::NewOrderLocalMemory::getCustomerOlderVersions() {
	return customerOlderVersions_;
}

RDMARegion<TPCC::DistrictVersion>* TPCC::NewOrderLocalMemory::getDistrictHead(){
	return districtHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getDistrictTS(){
	return districtTS_;
}
RDMARegion<TPCC::DistrictVersion>* TPCC::NewOrderLocalMemory::getDistrictOlderVersions(){
	return districtOlderVersions_;
}

RDMARegion<TPCC::ItemVersion>* TPCC::NewOrderLocalMemory::getItemHead(){
	return itemHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getItemTS(){
	return itemTS_;
}
RDMARegion<TPCC::ItemVersion>* TPCC::NewOrderLocalMemory::getItemOlderVersions(){
	return itemOlderVersions_;
}

RDMARegion<TPCC::NewOrderVersion>* TPCC::NewOrderLocalMemory::getNewOrderHead(){
	return newOrderHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getNewOrderTS(){
	return newOrderTS_;
}
RDMARegion<TPCC::NewOrderVersion>* TPCC::NewOrderLocalMemory::getNewOrderOlderVersions(){
	return newOrderOlderVersions_;
}

RDMARegion<TPCC::OrderLineVersion>* TPCC::NewOrderLocalMemory::getOrderLineHead(){
	return orderLineHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getOrderLineTS(){
	return orderLineTS_;
}
RDMARegion<TPCC::OrderLineVersion>* TPCC::NewOrderLocalMemory::getOrderLineOlderVersions(){
	return orderLineOlderVersions_;
}

RDMARegion<TPCC::OrderVersion>* TPCC::NewOrderLocalMemory::getOrderHead(){
	return orderHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getOrderTS(){
	return orderTS_;
}
RDMARegion<TPCC::OrderVersion>* TPCC::NewOrderLocalMemory::getOrderOlderVersions(){
	return orderOlderVersions_;
}

RDMARegion<TPCC::StockVersion>* TPCC::NewOrderLocalMemory::getStockHead(){
	return stockHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getStockTS(){
	return stockTS_;
}
RDMARegion<TPCC::StockVersion>* TPCC::NewOrderLocalMemory::getStockOlderVersions(){
	return stockOlderVersions_;
}

RDMARegion<TPCC::WarehouseVersion>* TPCC::NewOrderLocalMemory::getWarehouseHead(){
	return warehouseHead_;
}
RDMARegion<Timestamp>* TPCC::NewOrderLocalMemory::getWarehouseTS(){
	return warehouseTS_;
}
RDMARegion<TPCC::WarehouseVersion>* TPCC::NewOrderLocalMemory::getWarehouseOlderVersions(){
	return warehouseOlderVersions_;
}

RDMARegion<uint64_t>* TPCC::NewOrderLocalMemory::getDistrictLockRegion(){
	return districtLockRegion_;
}

RDMARegion<uint64_t>* TPCC::NewOrderLocalMemory::getStocksLocksRegion(){
	return stocksLocksRegion_;
}


TPCC::NewOrderLocalMemory::~NewOrderLocalMemory(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	delete customerHead_;
	delete customerTS_;
	delete customerOlderVersions_;

	delete districtHead_;
	delete districtTS_;
	delete districtOlderVersions_;

	delete itemHead_;
	delete itemTS_;
	delete itemOlderVersions_;

	delete newOrderHead_;
	delete newOrderTS_;
	delete newOrderOlderVersions_;

	delete orderLineHead_;
	delete orderLineTS_;
	delete orderLineOlderVersions_;

	delete orderHead_;
	delete orderTS_;
	delete orderOlderVersions_;

	delete stockHead_;
	delete stockTS_;
	delete stockOlderVersions_;

	delete warehouseHead_;
	delete warehouseTS_;
	delete warehouseOlderVersions_;

	delete districtLockRegion_;
	delete stocksLocksRegion_;
}
