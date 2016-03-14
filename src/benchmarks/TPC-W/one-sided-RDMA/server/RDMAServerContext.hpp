/*
 *	RDMAServerContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_SERVER_CONTEXT_H_
#define RDMA_SERVER_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../../../config.hpp"
#include "../../util/RDMACommon.hpp"
#include "../../basic-types/timestamp.hpp"
#include "../../tables/cc_xacts_version.hpp"
#include "../../tables/item_version.hpp"
#include "../../tables/order_line_version.hpp"
#include "../../tables/orders_version.hpp"
#include "../../tables/shopping_cart_line.hpp"
#include "../shared/TPCW_ServerMemoryKeys.hpp"

class RDMAServerContext : public BaseContext {
public:
	std::string	server_address;
	ShoppingCartLine	*associated_cart_line;
	
	// memory handlers
	struct ibv_mr *send_mr;
	struct ibv_mr *mr_items_head;
	//struct ibv_mr *mr_orders;
	//struct ibv_mr *mr_order_line;
	//struct ibv_mr *mr_cc_xacts;
	struct ibv_mr *mr_items_older_versions;
	struct ibv_mr *mr_items_pointer_list;

	// memory buffers
	struct message::DataServerMemoryKeys send_msg;
	ItemVersion			*items_head;
	//OrdersVersion		*orders_region;
	//OrderLineVersion	*order_line_region;
	//CCXactsVersion		*cc_xacts_region;

	ItemVersion		*items_older_versions;
	Timestamp		*items_pointer_list;

	int register_memory();
	int destroy_context ();
};
#endif // RDMA_SERVER_CONTEXT_H_
