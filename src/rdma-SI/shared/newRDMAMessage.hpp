/*
 *	RDMAMessage.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_MESSAGE_H_
#define RDMA_MESSAGE_H_

#include "../../auxilary/timestamp.hpp"
#include <infiniband/verbs.h>

namespace message {
	struct DataServerMemoryKeys {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes
		struct ibv_mr mr_items;
		struct ibv_mr mr_orders;
		struct ibv_mr mr_order_line;
		struct ibv_mr mr_cc_xacts;
		struct ibv_mr mr_timestamp;
		struct ibv_mr mr_lock_items;
	};

	struct TimestampServerMemoryKeys {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes
		struct ibv_mr mr_finished_trxs_hash;
		struct ibv_mr mr_read_timestamp;
		struct ibv_mr mr_commit_timestamp;
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
