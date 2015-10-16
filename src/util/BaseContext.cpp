/*
 *	BaseContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "BaseContext.hpp"
#include "utils.hpp"
#include "RDMACommon.hpp"
#include <unistd.h>		// for close


#include <iostream>

int BaseContext::create_context() {
	TEST_NZ (RDMACommon::build_connection(ib_port, &ib_ctx, &port_attr, &pd, &send_cq, &recv_cq, &send_comp_channel,&recv_comp_channel, 10));
	TEST_NZ (register_memory());
	TEST_NZ (RDMACommon::create_queuepair(ib_ctx, pd, send_cq, recv_cq, &qp));
	return 0;
}

int BaseContext::destroy_context() {
	if (send_cq)				TEST_NZ (ibv_destroy_cq (send_cq));
	if (recv_cq)				TEST_NZ (ibv_destroy_cq (recv_cq));
	if (pd)						TEST_NZ (ibv_dealloc_pd (pd));
	if (ib_ctx)					TEST_NZ (ibv_close_device (ib_ctx));
	if (sockfd >= 0)			TEST_NZ (close (sockfd));
	return 0;
}
