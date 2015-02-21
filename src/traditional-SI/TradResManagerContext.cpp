/*
 *	TradResManagerContext.cpp
 *
 *	Created on: 20.Feb.2015
 *	Author: erfanz
 */

#include "TradResManagerContext.hpp"
#include "../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int TradResManagerContext::register_memory() {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;		
	
	TEST_Z(item_info_req_mr		= ibv_reg_mr(pd, &item_info_request, sizeof(struct ItemInfoRequest), mr_flags));
	TEST_Z(item_info_res_mr		= ibv_reg_mr(pd, &item_info_response, sizeof(struct ItemInfoResponse), mr_flags));
	TEST_Z(lock_req_mr			= ibv_reg_mr(pd, &lock_request, sizeof(struct LockRequest), mr_flags));
	TEST_Z(lock_res_mr			= ibv_reg_mr(pd, &lock_response, sizeof(struct LockResponse), mr_flags));
	TEST_Z(write_data_req_mr	= ibv_reg_mr(pd, &write_data_request, sizeof(struct WriteDataRequest), mr_flags));
	return 0;
}

int TradResManagerContext::destroy_context () {
	if (qp) 				TEST_NZ(ibv_destroy_qp (qp));
	if (item_info_req_mr) 	TEST_NZ (ibv_dereg_mr (item_info_req_mr));
	if (item_info_res_mr) 	TEST_NZ (ibv_dereg_mr (item_info_res_mr));
	if (lock_req_mr) 		TEST_NZ (ibv_dereg_mr (lock_req_mr));
	if (lock_res_mr)		TEST_NZ (ibv_dereg_mr (lock_res_mr));
	if (write_data_req_mr)	TEST_NZ (ibv_dereg_mr (write_data_req_mr));
	if (cq)					TEST_NZ (ibv_destroy_cq (cq));
	if (pd)					TEST_NZ (ibv_dealloc_pd (pd));
	if (ib_ctx)				TEST_NZ (ibv_close_device (ib_ctx));
	return 0;
}