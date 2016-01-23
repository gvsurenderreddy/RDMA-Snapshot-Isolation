/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

#include "../../../config.hpp"
#include "../../util/RDMACommon.hpp"
#include "../../basic-types/PrimitiveTypes.hpp"
#include "DataServerContext.hpp"
#include "TimestampServerContext.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


typedef message::TransactionResult::Result Result;


class RDMAClient{
private:
	primitive::client_id_t	client_id_;
	size_t	client_cnt_;
	DataServerContext ds_ctx_[config::SERVER_CNT];
	TimestampServerContext ts_ctx_;
	primitive::timestamp_t	next_epoch_;

	struct Cart {
		ShoppingCartLine	cart_lines[config::ORDERLINE_PER_ORDER];
	} cart_;

	void	fill_shopping_cart();
	int		acquire_read_ts();
	int		get_head_version(const size_t server_num);
	int		get_versions_pointers(const size_t server_num);
	int 	acquire_commit_ts();
	int		acquire_item_lock(const size_t server_num, Timestamp &expectedTS, Timestamp &newTS);
	int		revert_lock(const size_t server_num, Timestamp &new_lock);
	int		append_pointer_to_pointer_list(const size_t server_num);
	int		append_version_to_versions(const size_t server_num);
	int		install_and_unlock(const size_t server_num);
	int		release_lock(const size_t server_num);
	int 	submit_trx_result();
	primitive::client_id_t	find_commiting_client(Timestamp &versionTimestamp);
	std::string pointer_to_string(const size_t server_num) const;
	std::string read_snapshot_to_string() const;



	int		startTransactions();

public:
	int start_client ();
	RDMAClient();
};

#endif /* RDMA_CLIENT_H_ */
