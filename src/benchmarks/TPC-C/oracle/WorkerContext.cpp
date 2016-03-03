/*
 * ClientContext.cpp
 *
 *  Created on: Feb 28, 2016
 *      Author: erfanz
 */

#include "../../../util/utils.hpp"
#include "WorkerContext.hpp"
#include <unistd.h>		// for close

#define CLASS_NAME "WorkerContext"

TPCC::WorkerContext::WorkerContext(int sockfd, RDMAContext &context)
: sockfd_(sockfd){
	memoryKeysMessage_ = new RDMARegion<OracleMemoryKeys>(1, context, IBV_ACCESS_LOCAL_WRITE);
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

RDMARegion<TPCC::OracleMemoryKeys>* TPCC::WorkerContext::getMemoryKeysMessage(){
	return memoryKeysMessage_;
}

int TPCC::WorkerContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* TPCC::WorkerContext::getQP() const{
	return qp_;
}

void TPCC::WorkerContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

TPCC::WorkerContext::~WorkerContext() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");
	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete memoryKeysMessage_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}