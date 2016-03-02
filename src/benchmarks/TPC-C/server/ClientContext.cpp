/*
 * ClientContext.cpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "ClientContext.hpp"
#include "../../../util/RDMACommon.hpp"
#include "../../../util/utils.hpp"	// for TEST_NZ()
#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME "ClientContext"

TPCC::ClientContext::ClientContext(int sockfd, RDMAContext &context)
: sockfd_(sockfd){
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
}

int TPCC::ClientContext::getSockFd() const{
	return sockfd_;
}

ibv_qp* TPCC::ClientContext::getQP() const{
	return qp_;
}

void TPCC::ClientContext::activateQueuePair(RDMAContext &context){
	TEST_NZ (RDMACommon::connect_qp (&qp_, context.getIbPort(), context.getPortAttr().lid, sockfd_));
}

TPCC::ClientContext::~ClientContext() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
