/*
 *	RDMAMessage.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: erfanz
 */

#ifndef RDMA_MESSAGE_H_
#define RDMA_MESSAGE_H_

#include <infiniband/verbs.h>

struct MemoryKeys {
	// We’ll use this to pass RDMA memory region (MR) keys between nodes
	struct ibv_mr mr_items;
	struct ibv_mr mr_orders;
	struct ibv_mr mr_order_line;
	struct ibv_mr mr_cc_xacts;
	struct ibv_mr mr_timestamp;
	struct ibv_mr mr_lock_items; 
};


#endif /* RDMA_MESSAGE_H_ */