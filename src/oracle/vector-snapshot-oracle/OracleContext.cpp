/*
 * OracleContext.cpp
 *
 *  Created on: Feb 29, 2016
 *      Author: erfanz
 */

#include "OracleContext.hpp"
#include "../../../config.hpp"
#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME	"OracleContext"

OracleContext::OracleContext(std::ostream &os, const int sockfd, const uint16_t tcpPort, const uint8_t ibPort, RDMAContext &context)
: os_(os),
  sockfd_(sockfd),
  tcpPort_(tcpPort),
  ibPort_(ibPort){
	peerMemoryKeys_			= new RDMARegion<OracleMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

uint16_t OracleContext::getTcpPort() const{
	return tcpPort_;
}

int OracleContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* OracleContext::getQP() const{
	return qp_;
}

std::set<primitive::client_id_t> OracleContext::getClientIDsInSnapshot() const{
	return clientIDsInSnapshot_;
}
void OracleContext::insertClientIDIntoSnapshot(primitive::client_id_t clientID){
	clientIDsInSnapshot_.insert(clientID);
}

void OracleContext::clearSnapshot(){
	clientIDsInSnapshot_.clear();
}


RDMARegion<OracleMemoryKeys>* OracleContext::getRemoteMemoryKeys(){
	return peerMemoryKeys_;
}

void OracleContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

OracleContext::~OracleContext(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");
	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete peerMemoryKeys_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
