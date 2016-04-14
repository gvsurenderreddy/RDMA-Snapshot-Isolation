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

TPCC::ServerContext::ServerContext(std::ostream &os, const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context)
: os_(os),
  sockfd_(sockfd),
  serverAddress_(serverAddress),
  tcpPort_(tcpPort),
  ibPort_(ibPort),
  instanceNum_(instanceNum){
	peerMemoryKeys_							= new RDMARegion<ServerMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);
	indexRequestMessage_ 					= new RDMARegion<TPCC::IndexRequestMessage>(1, context, IBV_ACCESS_LOCAL_WRITE);
	indexResponseMessage_ 					= new RDMARegion<TPCC::IndexResponseMessage>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerNameIndexRespMsg_ 				= new RDMARegion<TPCC::CustomerNameIndexRespMsg>(1, context, IBV_ACCESS_LOCAL_WRITE);
	largestOrderForCustomerIndexRespMsg_ 	= new RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>(1, context, IBV_ACCESS_LOCAL_WRITE);
	last20OrdersIndexRespMsg_				= new RDMARegion<TPCC::Last20OrdersIndexResMsg>(1, context, IBV_ACCESS_LOCAL_WRITE);

	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));

}

std::string TPCC::ServerContext::getServerAddress() const{
	return serverAddress_;
}

uint16_t TPCC::ServerContext::getTcpPort() const{
	return tcpPort_;
}

int TPCC::ServerContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* TPCC::ServerContext::getQP() const{
	return qp_;
}

RDMARegion<TPCC::ServerMemoryKeys>* TPCC::ServerContext::getRemoteMemoryKeys(){
	return peerMemoryKeys_;
}

RDMARegion<TPCC::IndexRequestMessage>* TPCC::ServerContext::getIndexRequestMessage(){
	return indexRequestMessage_;
}

RDMARegion<TPCC::IndexResponseMessage>* TPCC::ServerContext::getRegisterOrderIndexResponseMessage(){
	return indexResponseMessage_;
}

RDMARegion<TPCC::CustomerNameIndexRespMsg>* TPCC::ServerContext::getCustomerNameIndexResponseMessage(){
	return customerNameIndexRespMsg_;
}

RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>* TPCC::ServerContext::getLargestOrderForCustomerIndexResponseMessage(){
	return largestOrderForCustomerIndexRespMsg_;
}

RDMARegion<TPCC::Last20OrdersIndexResMsg>* TPCC::ServerContext::getLast20OrdersIndexResponseMessage(){
	return last20OrdersIndexRespMsg_;
}

void TPCC::ServerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

TPCC::ServerContext::~ServerContext(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete peerMemoryKeys_;
	delete indexRequestMessage_;
	delete indexResponseMessage_;
	delete customerNameIndexRespMsg_;
	delete largestOrderForCustomerIndexRespMsg_;
	delete last20OrdersIndexRespMsg_;

	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
