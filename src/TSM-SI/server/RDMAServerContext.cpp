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

	size_t i_s	= config::ITEM_CNT * sizeof(ItemVersion);
	//size_t o_s	= config::MAX_ORDERS_CNT * sizeof(OrdersVersion);	// TODO
	//size_t ol_s	= config::ORDERLINE_PER_ORDER * config::MAX_ORDERS_CNT * sizeof(OrderLineVersion);
	//size_t cc_s	= config::MAX_CCXACTS_CNT * sizeof(CCXactsVersion); 	// TODO
	size_t ov_s	= config::ITEM_CNT * config::MAX_ITEM_VERSIONS * sizeof(ItemVersion);
	size_t pl_s	= config::ITEM_CNT * config::MAX_ITEM_VERSIONS * sizeof(Timestamp);

	TEST_Z(send_mr			= ibv_reg_mr(pd, &send_msg, sizeof(struct message::DataServerMemoryKeys), mr_flags));
	TEST_Z(mr_items_head	= ibv_reg_mr(pd, items_head, i_s, mr_flags));
	//TEST_Z(mr_orders		= ibv_reg_mr(pd, orders_region, o_s, mr_flags));
	//TEST_Z(mr_order_line	= ibv_reg_mr(pd, order_line_region, ol_s, mr_flags));
	//TEST_Z(mr_cc_xacts		= ibv_reg_mr(pd, cc_xacts_region, cc_s, mr_flags));
	TEST_Z(mr_items_older_versions	= ibv_reg_mr(pd, items_older_versions, ov_s, mr_flags));
	TEST_Z(mr_items_pointer_list	= ibv_reg_mr(pd, items_pointer_list, pl_s, mr_flags));

	return 0;
}

int RDMAServerContext::destroy_context () {
	if (qp) TEST_NZ(ibv_destroy_qp (qp));
	if (send_mr) TEST_NZ (ibv_dereg_mr (send_mr));
	if (mr_items_head) TEST_NZ (ibv_dereg_mr (mr_items_head));
	//if (mr_orders) TEST_NZ (ibv_dereg_mr (mr_orders));
	//if (mr_order_line) TEST_NZ (ibv_dereg_mr (mr_order_line));
	//if (mr_cc_xacts) TEST_NZ (ibv_dereg_mr (mr_cc_xacts));
	if (mr_items_older_versions) TEST_NZ (ibv_dereg_mr (mr_items_older_versions));
	if (mr_items_pointer_list) TEST_NZ (ibv_dereg_mr (mr_items_pointer_list));
	
	this->BaseContext::destroy_context();
	return 0;
}
