/*
 *	RDMAMessage.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_MESSAGE_H_
#define RDMA_MESSAGE_H_

#include "../../basic-types/timestamp.hpp"
#include "../../basic-types/PrimitiveTypes.hpp"
#include "../../tpcw-tables/item_version.hpp"
#include <infiniband/verbs.h>

namespace message {
	struct DataServerMemoryKeys {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes
		struct ibv_mr mr_items_head;
		//struct ibv_mr mr_orders;
		//struct ibv_mr mr_order_line;
		//struct ibv_mr mr_cc_xacts;
		//struct ibv_mr mr_timestamp;
		struct ibv_mr mr_items_pointer_list;
		struct ibv_mr mr_items_older_versions;

		ItemVersion		*global_items_head;
		Timestamp		*global_items_pointer_list;
		ItemVersion		*global_items_older_versions;

		unsigned server_instance_num;
	};

	struct TimestampServerMemoryKeys {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes
		//struct ibv_mr mr_finished_trxs_hash;
		//struct ibv_mr mr_read_trx;
		struct ibv_mr mr_last_trx_timestamp_per_client;
		primitive::client_id_t client_id;
		size_t client_cnt;
	};

	struct TransactionResult {
		enum Result {
			COMMITTED,
			ABORTED
		} result;
		Timestamp ts;
	};
}

#endif /* RDMA_MESSAGE_H_ */
