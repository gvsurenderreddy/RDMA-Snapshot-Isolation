/*
 *	RDMAClientContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "ServerContext.hpp"

#include "../../../../config.hpp"
#include "../../../util/utils.hpp"

#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME	"ServerContext"

ServerContext::ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context)
: sockfd_(sockfd),
  serverAddress_(serverAddress),
  tcpPort_(tcpPort),
  ibPort_(ibPort),
  instanceNum_(instanceNum){
	peerMemoryKeys_			= new RDMARegion<ServerMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localCustomerHead_			= new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localCustomerTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localCustomerOlderVersions_ = new RDMARegion<TPCC::CustomerVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localDistrictHead_			= new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localDistrictTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localDistrictOlderVersions_ = new RDMARegion<TPCC::DistrictVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localItemHead_			= new RDMARegion<TPCC::ItemVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localItemTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localItemOlderVersions_ = new RDMARegion<TPCC::ItemVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localNewOrderHead_			= new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localNewOrderTS_			= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localNewOrderOlderVersions_ = new RDMARegion<TPCC::NewOrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localOrderLineHead_				= new RDMARegion<TPCC::OrderLineVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localOrderLineTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localOrderLineOlderVersions_ 	= new RDMARegion<TPCC::OrderLineVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localOrderHead_				= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localOrderTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localOrderOlderVersions_ 	= new RDMARegion<TPCC::OrderVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localStockHead_				= new RDMARegion<TPCC::StockVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localStockTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localStockOlderVersions_ 	= new RDMARegion<TPCC::StockVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	localWarehouseHead_				= new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);
	localWarehouseTS_				= new RDMARegion<Timestamp>(config::tpcc_settings::VERSION_NUM, context, IBV_ACCESS_LOCAL_WRITE);
	localWarehouseOlderVersions_ 	= new RDMARegion<TPCC::WarehouseVersion>(1, context, IBV_ACCESS_LOCAL_WRITE);

	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

std::string ServerContext::getServerAddress() const{
	return serverAddress_;
}

uint16_t ServerContext::getTcpPort() const{
	return tcpPort_;
}

int ServerContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* ServerContext::getQP() const{
	return qp_;
}

RDMARegion<ServerMemoryKeys>* ServerContext::getRemoteMemoryKeys(){
	return peerMemoryKeys_;
}

void ServerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

RDMARegion<TPCC::CustomerVersion>* ServerContext::getLocalCustomerHead() {
	return localCustomerHead_;
}

RDMARegion<Timestamp>* ServerContext::getLocalCustomerTS() {
	return localCustomerTS_;
}

RDMARegion<TPCC::CustomerVersion>* ServerContext::getLocalCustomerOlderVersions() {
	return localCustomerOlderVersions_;
}

RDMARegion<TPCC::DistrictVersion>* ServerContext::getLocalDistrictHead(){
	return localDistrictHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalDistrictTS(){
	return localDistrictTS_;
}
RDMARegion<TPCC::DistrictVersion>* ServerContext::getLocalDistrictOlderVersions(){
	return localDistrictOlderVersions_;
}

RDMARegion<TPCC::ItemVersion>* ServerContext::getLocalItemHead(){
	return localItemHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalItemTS(){
	return localItemTS_;
}
RDMARegion<TPCC::ItemVersion>* ServerContext::getLocalItemOlderVersions(){
	return localItemOlderVersions_;
}

RDMARegion<TPCC::NewOrderVersion>* ServerContext::getLocalNewOrderHead(){
	return localNewOrderHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalNewOrderTS(){
	return localNewOrderTS_;
}
RDMARegion<TPCC::NewOrderVersion>* ServerContext::getLocalNewOrderOlderVersions(){
	return localNewOrderOlderVersions_;
}

RDMARegion<TPCC::OrderLineVersion>* ServerContext::getLocalOrderLineHead(){
	return localOrderLineHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalOrderLineTS(){
	return localOrderLineTS_;
}
RDMARegion<TPCC::OrderLineVersion>* ServerContext::getLocalOrderLineOlderVersions(){
	return localOrderLineOlderVersions_;
}

RDMARegion<TPCC::OrderVersion>* ServerContext::getLocalOrderHead(){
	return localOrderHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalOrderTS(){
	return localOrderTS_;
}
RDMARegion<TPCC::OrderVersion>* ServerContext::getLocalOrderOlderVersions(){
	return localOrderOlderVersions_;
}

RDMARegion<TPCC::StockVersion>* ServerContext::getLocalStockHead(){
	return localStockHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalStockTS(){
	return localStockTS_;
}
RDMARegion<TPCC::StockVersion>* ServerContext::getLocalStockOlderVersions(){
	return localStockOlderVersions_;
}

RDMARegion<TPCC::WarehouseVersion>* ServerContext::getLocalWarehouseHead(){
	return localWarehouseHead_;
}
RDMARegion<Timestamp>* ServerContext::getLocalWarehouseTS(){
	return localWarehouseTS_;
}
RDMARegion<TPCC::WarehouseVersion>* ServerContext::getLocalWarehouseOlderVersions(){
	return localWarehouseOlderVersions_;
}


ServerContext::~ServerContext(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete peerMemoryKeys_;

	delete localCustomerHead_;
	delete localCustomerTS_;
	delete localCustomerOlderVersions_;

	delete localDistrictHead_;
	delete localDistrictTS_;
	delete localDistrictOlderVersions_;

	delete localItemHead_;
	delete localItemTS_;
	delete localItemOlderVersions_;

	delete localNewOrderHead_;
	delete localNewOrderTS_;
	delete localNewOrderOlderVersions_;

	delete localOrderLineHead_;
	delete localOrderLineTS_;
	delete localOrderLineOlderVersions_;

	delete localOrderHead_;
	delete localOrderTS_;
	delete localOrderOlderVersions_;

	delete localStockHead_;
	delete localStockTS_;
	delete localStockOlderVersions_;

	delete localWarehouseHead_;
	delete localWarehouseTS_;
	delete localWarehouseOlderVersions_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
