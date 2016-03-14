/*
 *	TradClientContext.cpp
 *
 *	Created on: 20.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../two-sided/TradClientContext.hpp"

#include "../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int TradClientContext::register_memory() {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;
	TEST_Z(commit_req_mr	= ibv_reg_mr(pd, &commit_request, sizeof(struct CommitRequest), mr_flags));
	TEST_Z(commit_res_mr	= ibv_reg_mr(pd, &commit_response, sizeof(struct CommitResponse), mr_flags));
	DEBUG_COUT("[Info] Memory registered");
	return 0;
}

int TradClientContext::destroy_context () {
	if (qp) 			TEST_NZ(ibv_destroy_qp (qp));
	if (commit_req_mr)	TEST_NZ (ibv_dereg_mr (commit_req_mr));
	if (commit_res_mr)	TEST_NZ (ibv_dereg_mr (commit_res_mr));
	if (cq)				TEST_NZ (ibv_destroy_cq (cq));
	if (pd)				TEST_NZ (ibv_dealloc_pd (pd));
	if (ib_ctx)			TEST_NZ (ibv_close_device (ib_ctx));
	if (sockfd >= 0)	TEST_NZ (close (sockfd));
	
	return 0;
}