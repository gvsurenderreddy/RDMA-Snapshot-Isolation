/*
 *	RDMAServerContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#ifndef RDMA_SERVER_CONTEXT_H_
#define RDMA_SERVER_CONTEXT_H_

#include "../util/BaseContext.hpp"
#include "RDMAMessage.hpp"
#include "../../config.hpp"
#include "../util/RDMACommon.hpp"
#include "../auxilary/timestamp.hpp"
#include "../auxilary/lock.hpp"
#include "../tables/cc_xacts_version.hpp"
#include "../tables/item_version.hpp"
#include "../tables/order_line_version.hpp"
#include "../tables/orders_version.hpp"
#include "../tables/shopping_cart_line.hpp"

class RDMAServerContext : public BaseContext {
public:
	std::string	server_address;
	
	ShoppingCartLine	*associated_cart_line;	
	
	
	// memory handlers
	struct ibv_mr *send_mr;
	struct ibv_mr *mr_items;
	struct ibv_mr *mr_orders;		
	struct ibv_mr *mr_order_line;
	struct ibv_mr *mr_cc_xacts;
	struct ibv_mr *mr_timestamp;
	struct ibv_mr *mr_lock_items;

	// memory buffers
	struct MemoryKeys 	send_msg;
	ItemVersion			*items_region;
	OrdersVersion		*orders_region;
	OrderLineVersion	*order_line_region;
	CCXactsVersion		*cc_xacts_region;
	Timestamp			*timestamp_region;
	uint64_t			*lock_items_region;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // RDMA_SERVER_CONTEXT_H_