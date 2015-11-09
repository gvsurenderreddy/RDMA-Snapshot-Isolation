/*
 *	RDMAServerContext.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>
#include "RDMAServerContext.hpp"

#define CLASS_NAME	"RDMAServerContext"


int RDMAServerContext::register_memory() {
	int mr_flags	=
		IBV_ACCESS_LOCAL_WRITE
			| IBV_ACCESS_REMOTE_READ
				| IBV_ACCESS_REMOTE_WRITE
					| IBV_ACCESS_REMOTE_ATOMIC;
	

	size_t i_s	= config::ITEM_CNT * config::MAX_ITEM_VERSIONS * sizeof(ItemVersion);
	
	//int o_s	= MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS * sizeof(OrdersVersion);
	size_t o_s	= config::MAX_BUFFER_SIZE * sizeof(OrdersVersion);	// TODO
	
	//int ol_s	= ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS * sizeof(OrderLineVersion);
	size_t ol_s	= config::MAX_BUFFER_SIZE * sizeof(OrderLineVersion);	// TODO
	
	//int cc_s	= MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS * sizeof(CCXactsVersion);
	size_t cc_s	= config::MAX_BUFFER_SIZE * sizeof(CCXactsVersion); 	// TODO

	size_t lock_item_s	= config::ITEM_CNT * sizeof(uint64_t);

	TEST_Z(send_mr			= ibv_reg_mr(pd, &send_msg, sizeof(struct message::DataServerMemoryKeys), mr_flags));
	TEST_Z(mr_items			= ibv_reg_mr(pd, items_region, i_s, mr_flags));
	TEST_Z(mr_orders		= ibv_reg_mr(pd, orders_region, o_s, mr_flags));
	TEST_Z(mr_order_line	= ibv_reg_mr(pd, order_line_region, ol_s, mr_flags));
	TEST_Z(mr_cc_xacts		= ibv_reg_mr(pd, cc_xacts_region, cc_s, mr_flags));
	TEST_Z(mr_lock_items	= ibv_reg_mr(pd, lock_items_region, lock_item_s, mr_flags));
	return 0;
}

int RDMAServerContext::destroy_context () {
	if (qp)
		TEST_NZ(ibv_destroy_qp (qp));
	
	if (send_mr)
		TEST_NZ (ibv_dereg_mr (send_mr));
	
	if (mr_items)
		TEST_NZ (ibv_dereg_mr (mr_items));
	
	if (mr_orders)
		TEST_NZ (ibv_dereg_mr (mr_orders));
	
	if (mr_order_line)
		TEST_NZ (ibv_dereg_mr (mr_order_line));
	
	if (mr_cc_xacts)
		TEST_NZ (ibv_dereg_mr (mr_cc_xacts));
	
	if (mr_lock_items)
		TEST_NZ (ibv_dereg_mr (mr_lock_items));
	
	this->BaseContext::destroy_context();
	return 0;
}
