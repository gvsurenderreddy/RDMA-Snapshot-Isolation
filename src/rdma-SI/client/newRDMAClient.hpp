/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#include "../../../config.hpp"
#include "../../util/RDMACommon.hpp"
#include "DataServerContext.hpp"
#include "TimestampServerContext.hpp"

typedef message::TransactionResult::Result Result;


class RDMAClient{
private:
	DataServerContext ds_ctx[config::SERVER_CNT];
	TimestampServerContext ts_ctx;


	struct Cart {
		ShoppingCartLine	cart_lines[config::ORDERLINE_PER_ORDER];
	} cart;

	void	fill_shopping_cart();
	
	int		acquire_read_ts();
	
	int		get_item_info(const size_t server_num);
	
	int 	acquire_commit_ts();
	
	int		acquire_item_lock(const size_t server_num, uint64_t *expected_lock, uint64_t *new_lock);
	
	int		revert_lock(const size_t server_num, uint64_t *new_lock);
	
	int		update_item_stock(const size_t server_num);
	
	int		release_lock(const size_t server_num);
	
	int 	submit_trx_result(Result result, int transactionNum);

	int		startTransactions();


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
};

#endif /* RDMA_CLIENT_H_ */
