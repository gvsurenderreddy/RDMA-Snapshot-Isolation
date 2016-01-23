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
#include "../../basic-types/timestamp.hpp"
#include "../shared/newRDMAMessage.hpp"

class DataServerContext : public BaseContext {
public:
	std::string	server_address;
	ShoppingCartLine	*associated_cart_line;	
	
	// local handler
	struct ibv_mr *mr_recv;
	struct ibv_mr *mr_items_head;
	struct ibv_mr *mr_items_older_version;
	struct ibv_mr *mr_items_pointer;

	//struct ibv_mr *mr_orders;
	//struct ibv_mr *mr_order_line;
	//struct ibv_mr *mr_cc_xacts;
	struct ibv_mr *mr_lock_item;
	
	// remote memory handlers
	struct ibv_mr peer_mr_items_head;
	//struct ibv_mr peer_mr_orders;
	//struct ibv_mr peer_mr_order_line;
	//struct ibv_mr peer_mr_cc_xacts;
	struct ibv_mr peer_mr_items_older_versions;
	struct ibv_mr peer_mr_items_pointer_list;

	
	// memory buffers
	struct message::DataServerMemoryKeys recv_msg;
	ItemVersion		*items_head;
	ItemVersion		*items_older_version;
	Timestamp		*items_pointer;

	//OrdersVersion		orders_region;
	//OrderLineVersion	*order_line_region;
	//CCXactsVersion		cc_xacts_region;
	uint64_t			lock_item_region;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // RDMA_CLIENT_CONTEXT_H_
