/*
 * BuyConfirmLocalMemory.cpp
 *
 *  Created on: Feb 26, 2016
 *      Author: erfanz
 */

#include "BuyConfirmLocalMemory.hpp"
#include "../../../../util/utils.hpp"
#include <infiniband/verbs.h>	// for ibv_qp

#define CLASS_NAME	"BuyConfirmLocalMemory"

TPCW::BuyConfirmLocalMemory::BuyConfirmLocalMemory(RDMAContext &context){
	itemHead_				= new RDMARegion<TPCW::ItemVersion>(config::tpcw_settings::ORDERLINE_PER_ORDER, context, IBV_ACCESS_LOCAL_WRITE);
	itemTS_					= new RDMARegion<Timestamp>(config::tpcw_settings::ORDERLINE_PER_ORDER * config::tpcw_settings::MAX_ITEM_VERSIONS, context, IBV_ACCESS_LOCAL_WRITE);
	itemOlderVersions_ 		= new RDMARegion<TPCW::ItemVersion>(config::tpcw_settings::ORDERLINE_PER_ORDER, context, IBV_ACCESS_LOCAL_WRITE);
	lockRegion_				= new RDMARegion<uint64_t>(config::tpcw_settings::ORDERLINE_PER_ORDER, context, IBV_ACCESS_LOCAL_WRITE);
}

RDMARegion<TPCW::ItemVersion>* TPCW::BuyConfirmLocalMemory::getItemHead(){
	return itemHead_;
}

RDMARegion<Timestamp>* TPCW::BuyConfirmLocalMemory::getItemTS(){
	return itemTS_;
}

RDMARegion<TPCW::ItemVersion>* TPCW::BuyConfirmLocalMemory::getItemOlderVersions(){
	return itemOlderVersions_;
}


RDMARegion<uint64_t>* TPCW::BuyConfirmLocalMemory::getLockRegion(){
	return lockRegion_;
}

TPCW::BuyConfirmLocalMemory::~BuyConfirmLocalMemory(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");
	delete itemHead_;
	delete itemTS_;
	delete itemOlderVersions_;
	delete lockRegion_;
}
