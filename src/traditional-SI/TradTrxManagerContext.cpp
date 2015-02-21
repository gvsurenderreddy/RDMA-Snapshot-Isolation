/*
 *	TradTrxManagerContext.cpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#include "TradTrxManagerContext.hpp"
#include "../util/utils.hpp"
#include "../util/RDMACommon.hpp"
#include <unistd.h>		// for close
#include <iostream>

// Override
int TradTrxManagerContext::create_context() {
	for (int s = 0; s < SERVER_CNT; s++) {
		TEST_NZ (res_mng_ctxs[s].create_context());
		TEST_NZ (RDMACommon::connect_qp (&(res_mng_ctxs[s].qp), res_mng_ctxs[s].ib_port, res_mng_ctxs[s].port_attr.lid, res_mng_ctxs[s].sockfd));
		DEBUG_COUT ("[Conn] Client is successfully connected to RM #" << s << ", QP #" << res_mng_ctxs[s].qp->qp_num);
	}
	TEST_NZ (client_ctx.create_context());
	DEBUG_COUT ("[Info] Context for client on sock " << client_ctx.sockfd << " is successfully created");
	
	TEST_NZ (RDMACommon::connect_qp (&(client_ctx.qp), client_ctx.ib_port, client_ctx.port_attr.lid, client_ctx.sockfd));
	DEBUG_COUT ("[Conn] TM QPed to client on sock " << client_ctx.sockfd);
}

int TradTrxManagerContext::register_memory() {
	;
}

int TradTrxManagerContext::destroy_context () {
	for (int i = 0; i < SERVER_CNT; i++) {
		if (res_mng_ctxs[i].qp) 				TEST_NZ(ibv_destroy_qp (res_mng_ctxs[i].qp));
		if (res_mng_ctxs[i].item_info_req_mr)	TEST_NZ (ibv_dereg_mr (res_mng_ctxs[i].item_info_req_mr));
		if (res_mng_ctxs[i].item_info_res_mr)	TEST_NZ (ibv_dereg_mr (res_mng_ctxs[i].item_info_res_mr));
		if (res_mng_ctxs[i].lock_req_mr) 		TEST_NZ (ibv_dereg_mr (res_mng_ctxs[i].lock_req_mr));
		if (res_mng_ctxs[i].lock_res_mr)		TEST_NZ (ibv_dereg_mr (res_mng_ctxs[i].lock_res_mr));
		if (res_mng_ctxs[i].write_data_req_mr)	TEST_NZ (ibv_dereg_mr (res_mng_ctxs[i].write_data_req_mr));
		if (res_mng_ctxs[i].cq)					TEST_NZ (ibv_destroy_cq (res_mng_ctxs[i].cq));
		if (res_mng_ctxs[i].pd)					TEST_NZ (ibv_dealloc_pd (res_mng_ctxs[i].pd));
	}
	
	TEST_NZ (client_ctx.destroy_context());	
	return 0;
}