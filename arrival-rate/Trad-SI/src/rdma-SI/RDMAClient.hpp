/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#include "RDMAClientContext.hpp"

#include "../../config.hpp"
#include "../util/RDMACommon.hpp"

class RDMAClient{
private:
	struct Cart {
		ShoppingCartLine	cart_lines[ORDERLINE_PER_ORDER];
	};
	
	struct SharedContext
	{
		uint64_t	read_timestamp;		
		uint64_t	commit_timestamp;
	};
	
	static Cart				cart;
	static SharedContext	shared_context;	

	static void		fill_shopping_cart(RDMAClientContext *ctx);
	
	static int		acquire_read_timestamp(RDMAClientContext *ctx, uint64_t *read_timestamp);
	
	static int		get_item_info(RDMAClientContext &ctx);
	
	static int 		acquire_commit_timestamp(RDMAClientContext &ctx, uint64_t *commit_timestamp);
	
	static int		acquire_item_lock(RDMAClientContext &ctx, uint64_t *expected_lock, uint64_t *new_lock);
	
	static int		revert_lock(RDMAClientContext &ctx, uint64_t *new_lock);
	
	static int		decrement_item_stock(RDMAClientContext &ctx);
	
	static int		release_lock(RDMAClientContext &ctx);
	
	static int		start_transaction(RDMAClientContext *ctx);


public:
	
	/******************************************************************************
	* Function: start_client
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the client. 
	*
	******************************************************************************/
	int start_client ();
	
	/******************************************************************************
	* Function: usage
	*
	* Input
	* argv0 command line arguments
	*
	* Output
	* none
	*
	* Returns
	* none
	*
	* Description
	* print a description of command line syntax
	******************************************************************************/
	static void usage (const char *argv0);
};

#endif /* RDMA_CLIENT_H_ */