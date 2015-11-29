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
	size_t i_s		= sizeof(ItemVersion);
	size_t iov_s	= sizeof(ItemVersion);
	size_t ip_s		= config::MAX_ITEM_VERSIONS * sizeof(Timestamp);

	//size_t o_s		= sizeof(OrdersVersion);
	//size_t ol_s		= config::ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);
	//size_t cc_s		= sizeof(CCXactsVersion);
	size_t lock_s	= sizeof(uint64_t);
	
	items_head		= new ItemVersion[1];
	//order_line_region	= new OrderLineVersion[config::ORDERLINE_PER_ORDER];
	items_older_version	= new ItemVersion[1];
	items_pointer		= new Timestamp[config::MAX_ITEM_VERSIONS];

	
	TEST_Z(mr_recv		= ibv_reg_mr(pd, &recv_msg, recv_s, mr_flags));
	TEST_Z(mr_items_head		= ibv_reg_mr(pd, items_head, i_s, mr_flags));
	TEST_Z(mr_items_older_version	= ibv_reg_mr(pd, items_older_version, iov_s, mr_flags));
	TEST_Z(mr_items_pointer			= ibv_reg_mr(pd, items_pointer, ip_s, mr_flags));

	//TEST_Z(mr_orders	= ibv_reg_mr(pd, &orders_region, o_s, mr_flags));
	//TEST_Z(mr_order_line= ibv_reg_mr(pd, order_line_region, ol_s, mr_flags));
	//TEST_Z(mr_cc_xacts	= ibv_reg_mr(pd, &cc_xacts_region, cc_s, mr_flags));
	TEST_Z(mr_lock_item = ibv_reg_mr(pd, &lock_item_region, lock_s, mr_flags));
	
	return 0;
}

int DataServerContext::destroy_context () {
	if (qp)				TEST_NZ(ibv_destroy_qp (qp));
	if (mr_recv)		TEST_NZ (ibv_dereg_mr (mr_recv));
	if (mr_items_head)		TEST_NZ (ibv_dereg_mr (mr_items_head));
	if (mr_items_older_version)		TEST_NZ (ibv_dereg_mr (mr_items_older_version));
	if (mr_items_pointer)			TEST_NZ (ibv_dereg_mr (mr_items_pointer));

	//if (mr_orders)		TEST_NZ (ibv_dereg_mr (mr_orders));
	//if (mr_order_line) 	TEST_NZ (ibv_dereg_mr (mr_order_line));
	//if (mr_cc_xacts)	TEST_NZ (ibv_dereg_mr (mr_cc_xacts));
	if (mr_lock_item) 	TEST_NZ (ibv_dereg_mr (mr_lock_item));

	delete[](items_head);
	delete[](items_older_version);
	delete[](items_pointer);

	//delete[](order_line_region);
	
	this->BaseContext::destroy_context();
	return 0;
}
