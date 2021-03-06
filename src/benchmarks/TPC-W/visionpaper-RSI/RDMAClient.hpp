/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

#include "RDMAClientContext.hpp"
#include "../../../../config.hpp"
#include "../../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

class RDMAClient{
private:
	struct Cart {
		ShoppingCartLine	cart_lines[config::tpcw_settings::ORDERLINE_PER_ORDER];
	};
	
	struct SharedContext
	{
		uint64_t	read_timestamp;		
		uint64_t	commit_timestamp;
	};
	
	static Cart				cart;
	static SharedContext	shared_context;	

	static int		check_if_version_is_in_block(int version, ItemVersion *items_region, int size);
	static void		fill_shopping_cart(RDMAClientContext *ctx);
	static void*	get_item_info(RDMAClientContext &ctx);
	static int 		acquire_commit_timestamp(RDMAClientContext &ctx, uint64_t *commit_timestamp);
	static void*	acquire_item_lock(RDMAClientContext &ctx, uint64_t &expected_lock, uint64_t &new_lock);
	static void*	revert_lock(RDMAClientContext &ctx, uint64_t *new_lock);
	static void*	decrement_item_stock(RDMAClientContext &ctx);
	static void*	release_lock(RDMAClientContext &ctx);
	static int		start_transaction(RDMAClientContext *ctx);

public:
	int start_client ();
	static void usage (const char *argv0);
};

#endif /* RDMA_CLIENT_H_ */
