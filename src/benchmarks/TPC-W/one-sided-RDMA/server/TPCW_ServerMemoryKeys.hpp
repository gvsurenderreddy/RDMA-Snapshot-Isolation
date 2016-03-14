/*
 *	RDMAMessage.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_MESSAGE_H_
#define RDMA_MESSAGE_H_

#include "../../../../basic-types/timestamp.hpp"
#include "../../../../rdma-region/MemoryHandler.hpp"
#include "../tables/item_version.hpp"
#include <infiniband/verbs.h>

namespace TPCW{
struct TPCW_ServerMemoryKeys{
	MemoryHandler<ItemVersion>	itemTableHeadVersions;
	MemoryHandler<Timestamp>	itemTableTimestampList;
	MemoryHandler<ItemVersion>	itemTableOlderVersions;

	unsigned server_instance_num;

	//	struct TimestampServerMemoryKeys {
	//		// We’ll use this to pass RDMA memory region (MR) keys between nodes
	//		//struct ibv_mr mr_finished_trxs_hash;
	//		//struct ibv_mr mr_read_trx;
	//		struct ibv_mr mr_last_trx_timestamp_per_client;
	//		primitive::client_id_t client_id;
	//		size_t client_cnt;
	//	};

//	struct TransactionResult {
//		enum Result {
//			COMMITTED,
//			ABORTED
//		} result;
//		Timestamp ts;
//	};
};	// struct TPCW_ServerMemoryKeys
}	// namespace TPCW

#endif /* RDMA_MESSAGE_H_ */
