/*
 *	RDMAClientContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../../one-sided-RDMA/client/TPCW_ServerContext.hpp"
#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME	"DSContext"


TPCW::TPCW_ServerContext::TPCW_ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context)
: sockfd_(sockfd),
  serverAddress_(serverAddress),
  tcpPort_(tcpPort),
  ibPort_(ibPort),
  instanceNum_(instanceNum){
	peerMemoryKeys_			= new RDMARegion<TPCW::TPCW_ServerMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

std::string TPCW::TPCW_ServerContext::getServerAddress() const{
	return serverAddress_;
}

uint16_t TPCW::TPCW_ServerContext::getTcpPort() const{
	return tcpPort_;
}

int TPCW::TPCW_ServerContext::getSockFd() const{
	return sockfd_;
}

unsigned TPCW::TPCW_ServerContext::getInstanceNum() const{
	return instanceNum_;

}

ibv_qp* TPCW::TPCW_ServerContext::getQP() const{
	return qp_;
}

RDMARegion<TPCW::TPCW_ServerMemoryKeys>* TPCW::TPCW_ServerContext::getRemoteMemoryKeys(){
	return peerMemoryKeys_;
}

void TPCW::TPCW_ServerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}



TPCW::TPCW_ServerContext::~TPCW_ServerContext(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete peerMemoryKeys_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
