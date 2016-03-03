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

TPCC::ServerContext::ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context)
: sockfd_(sockfd),
  serverAddress_(serverAddress),
  tcpPort_(tcpPort),
  ibPort_(ibPort),
  instanceNum_(instanceNum){
	peerMemoryKeys_			= new RDMARegion<ServerMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);

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

void TPCC::ServerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

TPCC::ServerContext::~ServerContext(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete peerMemoryKeys_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}