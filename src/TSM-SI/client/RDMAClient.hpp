/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

//#include <byteswap.h>
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
	size_t	client_id_;
	size_t	client_cnt_;
	DataServerContext ds_ctx_[config::SERVER_CNT];
	TimestampServerContext ts_ctx_;
	Timestamp	next_epoch_;

	struct Cart {
		ShoppingCartLine	cart_lines[config::ORDERLINE_PER_ORDER];
	} cart_;

	void	fill_shopping_cart();
	int		acquire_read_ts();
	int		get_item_info(const size_t server_num);
	int 	acquire_commit_ts();
	int		acquire_item_lock(const size_t server_num, Timestamp &expectedTS, Timestamp &newTS);
	int		revert_lock(const size_t server_num, Timestamp &new_lock);
	int		install_and_unlock(const size_t server_num);
	int		release_lock(const size_t server_num);
	int 	submit_trx_result();
	int		startTransactions();
	bool 	check_if_wrapped_sweeper() const;

public:
	int start_client ();
	RDMAClient();
};

#endif /* RDMA_CLIENT_H_ */
