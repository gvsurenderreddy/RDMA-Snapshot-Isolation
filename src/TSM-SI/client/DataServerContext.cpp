/*
 *	RDMAClientContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "DataServerContext.hpp"

#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

#define CLASS_NAME	"DSContext"


int DataServerContext::register_memory() {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	size_t recv_s	= sizeof(struct message::DataServerMemoryKeys);
	size_t i_s		= config::FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	size_t o_s		= sizeof(OrdersVersion);
	size_t ol_s		= config::ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);
	size_t cc_s		= sizeof(CCXactsVersion);
	size_t ts_s		= sizeof(Timestamp);
	size_t lock_s	= sizeof(uint64_t);
	
	items_region		= new ItemVersion[config::FETCH_BLOCK_SIZE];
	order_line_region	= new OrderLineVersion[config::ORDERLINE_PER_ORDER];
	lock_items_region	= new uint64_t[1];	
	
	TEST_Z(mr_recv		= ibv_reg_mr(pd, &recv_msg, recv_s, mr_flags));
	TEST_Z(mr_items		= ibv_reg_mr(pd, items_region, i_s, mr_flags));
	TEST_Z(mr_orders	= ibv_reg_mr(pd, &orders_region, o_s, mr_flags));
	TEST_Z(mr_order_line= ibv_reg_mr(pd, order_line_region, ol_s, mr_flags));
	TEST_Z(mr_cc_xacts	= ibv_reg_mr(pd, &cc_xacts_region, cc_s, mr_flags));
	TEST_Z(mr_read_ts	= ibv_reg_mr(pd, &read_ts_region, ts_s, mr_flags));
	TEST_Z(mr_commit_ts	= ibv_reg_mr(pd, &commit_ts_region, ts_s, mr_flags));
	TEST_Z(mr_lock_items= ibv_reg_mr(pd, lock_items_region, lock_s, mr_flags));
	
	return 0;
}

int DataServerContext::destroy_context () {
	if (qp)				TEST_NZ(ibv_destroy_qp (qp));
	if (mr_recv)		TEST_NZ (ibv_dereg_mr (mr_recv));
	if (mr_items)		TEST_NZ (ibv_dereg_mr (mr_items));
	if (mr_orders)		TEST_NZ (ibv_dereg_mr (mr_orders));
	if (mr_order_line) 	TEST_NZ (ibv_dereg_mr (mr_order_line));
	if (mr_cc_xacts)	TEST_NZ (ibv_dereg_mr (mr_cc_xacts));
	if (mr_read_ts)		TEST_NZ (ibv_dereg_mr (mr_read_ts));
	if (mr_commit_ts)	TEST_NZ (ibv_dereg_mr (mr_commit_ts));
	if (mr_lock_items) 	TEST_NZ (ibv_dereg_mr (mr_lock_items));
	
	delete[](items_region);
	delete[](order_line_region);
	delete[](lock_items_region);
	
	this->BaseContext::destroy_context();
	return 0;
}
