/*
 * WorkerContext.cpp
 *
 *  Created on: Feb 28, 2016
 *      Author: erfanz
 */

#include "../util/utils.hpp"
#include "WorkerContext.hpp"
#include <unistd.h>		// for close

#define CLASS_NAME "WorkerContext"

WorkerContext::WorkerContext(std::ostream &os, int sockfd, RDMAContext &context)
: os_(os),
  sockfd_(sockfd){
	memoryKeysMessage_ = new RDMARegion<OracleMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

RDMARegion<OracleMemoryKeys>* WorkerContext::getMemoryKeysMessage(){
	return memoryKeysMessage_;
}

int WorkerContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* WorkerContext::getQP() const{
	return qp_;
}

void WorkerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

WorkerContext::~WorkerContext() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");
	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete memoryKeysMessage_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
