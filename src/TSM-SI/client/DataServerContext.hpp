/*
 *	RDMAClientContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_CONTEXT_H_
#define RDMA_CLIENT_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../../../config.hpp"
#include "../../util/RDMACommon.hpp"
#include "../../tpcw-tables/item_version.hpp"
#include "../../tpcw-tables/orders_version.hpp"
#include "../../tpcw-tables/order_line_version.hpp"
#include "../../tpcw-tables/cc_xacts_version.hpp"
#include "../../tpcw-tables/shopping_cart_line.hpp"
#include "../../auxilary/timestamp.hpp"
#include "../../auxilary/lock.hpp"
#include "../shared/newRDMAMessage.hpp"

class DataServerContext : public BaseContext {
public:
	std::string	server_address;

	ShoppingCartLine	*associated_cart_line;	
	
	// local handler
	struct ibv_mr *mr_recv;
	struct ibv_mr *mr_items;
	struct ibv_mr *mr_orders;
	struct ibv_mr *mr_order_line;
	struct ibv_mr *mr_cc_xacts;
	struct ibv_mr *mr_read_ts;
	struct ibv_mr *mr_commit_ts;
	struct ibv_mr *mr_lock_items;
	
	// remote memory handlers
	struct ibv_mr peer_mr_items;
	struct ibv_mr peer_mr_orders;
	struct ibv_mr peer_mr_order_line;
	struct ibv_mr peer_mr_cc_xacts;
	struct ibv_mr peer_mr_timestamp;
	struct ibv_mr peer_mr_lock_items;
	
	// memory buffers
	struct message::DataServerMemoryKeys recv_msg;
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
