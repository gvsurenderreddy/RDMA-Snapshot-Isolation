/*
 *	BaseContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "RDMAContext.hpp"
#include "RDMACommon.hpp"
#include "../util/utils.hpp"
#include <unistd.h>		// for close

#include <iostream>

#define CLASS_NAME "RDMAContext"

RDMAContext::RDMAContext(std::ostream &os, uint8_t ib_port)
: os_(os),
  ib_port_(ib_port){
	TEST_NZ (RDMACommon::build_connection(ib_port_, &ib_ctx_, &port_attr_, &pd_, &send_cq_, &recv_cq_, &send_comp_channel_,&recv_comp_channel_, 50000));
}

uint8_t RDMAContext::getIbPort(){
	return ib_port_;
}

ibv_context* RDMAContext::getIbCtx(){
	return ib_ctx_;
}

ibv_pd* RDMAContext::getPd(){
	return pd_;
}

ibv_cq* RDMAContext::getSendCq(){
	return send_cq_;
}

ibv_cq* RDMAContext::getRecvCq(){
	return recv_cq_;
}

ibv_port_attr RDMAContext::getPortAttr(){
	return port_attr_;
}

RDMAContext::~RDMAContext(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");
	if (send_cq_)	TEST_NZ (ibv_destroy_cq (send_cq_));
	if (recv_cq_)	TEST_NZ (ibv_destroy_cq (recv_cq_));
	if (pd_)		TEST_NZ (ibv_dealloc_pd (pd_));
	if (ib_ctx_)	TEST_NZ (ibv_close_device (ib_ctx_));
}
