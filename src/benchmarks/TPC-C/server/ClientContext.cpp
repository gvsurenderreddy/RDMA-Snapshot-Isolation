/*
 * ClientContext.cpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "ClientContext.hpp"
#include "../../../rdma-region/RDMACommon.hpp"
#include "../../../util/utils.hpp"	// for TEST_NZ()
#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME "ClientContext"

TPCC::ClientContext::ClientContext(std::ostream &os, int sockfd, RDMAContext &context)
: os_(os),
  sockfd_(sockfd){
	TEST_NZ (RDMACommon::create_queuepair(context.getIbCtx(), context.getPd(), context.getSendCq(), context.getRecvCq(), &qp_));
	indexRequestMessage_ 				= new RDMARegion<TPCC::IndexRequestMessage>(1, context, IBV_ACCESS_LOCAL_WRITE);
	indexResponseMessage_ 				= new RDMARegion<TPCC::IndexResponseMessage>(1, context, IBV_ACCESS_LOCAL_WRITE);
	customerNameIndexResponseMessage_ 	= new RDMARegion<TPCC::CustomerNameIndexRespMsg>(1, context, IBV_ACCESS_LOCAL_WRITE);
	largestOrderIndexResponseMessage_ 	= new RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>(1, context, IBV_ACCESS_LOCAL_WRITE);
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

RDMARegion<TPCC::IndexRequestMessage>* TPCC::ClientContext::getIndexRequestMessage() const{
	return indexRequestMessage_;
}

RDMARegion<TPCC::IndexResponseMessage>* TPCC::ClientContext::getIndexResponseMessage() const{
	return indexResponseMessage_;
}

RDMARegion<TPCC::CustomerNameIndexRespMsg>* TPCC::ClientContext::getCustomerNameIndexResponseMessage() const{
	return customerNameIndexResponseMessage_;
}

RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>* TPCC::ClientContext::getLargestOrderForCustomerIndexResponseMessage() const{
	return largestOrderIndexResponseMessage_;
}

TPCC::ClientContext::~ClientContext() {
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");

	if (qp_) TEST_NZ(ibv_destroy_qp (qp_));
	delete indexRequestMessage_;
	delete indexResponseMessage_;
	delete customerNameIndexResponseMessage_;
	delete largestOrderIndexResponseMessage_;
	if (sockfd_ >= 0) TEST_NZ (close (sockfd_));
}
