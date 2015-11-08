/*
 *	RDMAClientContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#ifndef RDMA_CLIENT_CONTEXT_H_
#define RDMA_CLIENT_CONTEXT_H_

#include "../util/BaseContext.hpp"
#include "RDMAMessage.hpp"
#include "../../config.hpp"
#include "../util/RDMACommon.hpp"
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders_version.hpp"
#include "../tpcw-tables/order_line_version.hpp"
#include "../tpcw-tables/cc_xacts_version.hpp"
#include "../tpcw-tables/shopping_cart_line.hpp"
#include "../auxilary/timestamp.hpp"
#include "../auxilary/lock.hpp"

class RDMAClientContext : public BaseContext {
public:
	std::string	server_address;
	
	ShoppingCartLine	*associated_cart_line;	
	
	// memory handler
	struct ibv_mr *recv_mr;			
	struct ibv_mr *items_mr;	
	struct ibv_mr *orders_mr;	
	struct ibv_mr *order_line_mr;	
	struct ibv_mr *cc_xacts_mr;	
	struct ibv_mr *read_ts_mr;	
	struct ibv_mr *commit_ts_mr;	
	struct ibv_mr *lock_items_mr;	
	
	// remote memory handlers
	struct ibv_mr peer_mr_items;
	struct ibv_mr peer_mr_orders;
	struct ibv_mr peer_mr_order_line;
	struct ibv_mr peer_mr_cc_xacts;
	struct ibv_mr peer_mr_timestamp;
	struct ibv_mr peer_mr_lock_items;
	
	// memory bufferes
	struct MemoryKeys	recv_msg;
	ItemVersion			*items_region;
	OrdersVersion		orders_region;
	OrderLineVersion	*order_line_region;
	CCXactsVersion		cc_xacts_region;
	Timestamp			read_ts_region;		
	Timestamp			commit_ts_region;
	uint64_t			*lock_items_region;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // RDMA_CLIENT_CONTEXT_H_