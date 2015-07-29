/*
 *	RDMAClientContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#include "RDMAClientContext.hpp"
#include "../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int RDMAClientContext::register_memory() {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	int recv_s	= sizeof(struct MemoryKeys);
	int i_s		= FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	int o_s		= sizeof(OrdersVersion);
	int ol_s	= ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);
	int cc_s	= sizeof(CCXactsVersion);
	int ts_s	= sizeof(Timestamp);
	int lock_s	= sizeof(uint64_t);
	
	items_region		= new ItemVersion[FETCH_BLOCK_SIZE];
	order_line_region	= new OrderLineVersion[ORDERLINE_PER_ORDER];
	lock_items_region	= new uint64_t[1];	
	
	TEST_Z(recv_mr			= ibv_reg_mr(pd, &recv_msg, recv_s, mr_flags));
	TEST_Z(items_mr			= ibv_reg_mr(pd, items_region, i_s, mr_flags));
	TEST_Z(orders_mr		= ibv_reg_mr(pd, &orders_region, o_s, mr_flags));
	TEST_Z(order_line_mr	= ibv_reg_mr(pd, order_line_region, ol_s, mr_flags));
	TEST_Z(cc_xacts_mr		= ibv_reg_mr(pd, &cc_xacts_region, cc_s, mr_flags));
	TEST_Z(read_ts_mr		= ibv_reg_mr(pd, &read_ts_region, ts_s, mr_flags));
	TEST_Z(commit_ts_mr		= ibv_reg_mr(pd, &commit_ts_region, ts_s, mr_flags));
	TEST_Z(lock_items_mr	= ibv_reg_mr(pd, lock_items_region, lock_s, mr_flags));
	
	return 0;
}

int RDMAClientContext::destroy_context () {
	if (qp)				TEST_NZ(ibv_destroy_qp (qp));
	if (recv_mr)		TEST_NZ (ibv_dereg_mr (recv_mr));
	if (items_mr)		TEST_NZ (ibv_dereg_mr (items_mr));
	if (orders_mr)		TEST_NZ (ibv_dereg_mr (orders_mr));
	if (order_line_mr) 	TEST_NZ (ibv_dereg_mr (order_line_mr));
	if (cc_xacts_mr)	TEST_NZ (ibv_dereg_mr (cc_xacts_mr));
	if (read_ts_mr)		TEST_NZ (ibv_dereg_mr (read_ts_mr));
	if (commit_ts_mr)	TEST_NZ (ibv_dereg_mr (commit_ts_mr));
	if (lock_items_mr) 	TEST_NZ (ibv_dereg_mr (lock_items_mr));
	
	delete[](items_region);
	delete[](order_line_region);
	delete[](lock_items_region);
	
	if (cq)						TEST_NZ (ibv_destroy_cq (cq));
	if (pd)						TEST_NZ (ibv_dealloc_pd (pd));
	if (ib_ctx)					TEST_NZ (ibv_close_device (ib_ctx));
	if (sockfd >= 0)			TEST_NZ (close (sockfd));
	
	return 0;
}