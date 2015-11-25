/*
 * TrxServerContext.hpp
 *
 *  Created on: Oct 4, 2015
 *      Author: erfanz
 */

#ifndef RDMA_NEWSI_CLIENT_TIMESTAMPSERVERCONTEXT_HPP_
#define RDMA_NEWSI_CLIENT_TIMESTAMPSERVERCONTEXT_HPP_

#include "../../util/BaseContext.hpp"
#include "../../../config.hpp"
#include "../../util/RDMACommon.hpp"
#include "../../auxilary/timestamp.hpp"
#include "../shared/newRDMAMessage.hpp"


class TimestampServerContext : public BaseContext {
public:
	std::string	server_address;


	// local memory handlers
	struct ibv_mr *mr_recv;					// handler for the RECEIVE message
	struct ibv_mr *mr_read_trx;
	struct ibv_mr *mr_trx_status;

	// local memory buffers
	struct message::TimestampServerMemoryKeys recv_msg;	// buffer for the RECEIVE message
	Timestamp	read_trx;
	Timestamp	commit_timestamp;
	uint8_t		trx_status;

	// remote memory handlers
	struct ibv_mr peer_mr_finished_trxs_hash;
	struct ibv_mr peer_mr_read_trx;

	int register_memory();
	int destroy_context ();
};

#endif /* RDMA_NEWSI_CLIENT_TRXSERVERCONTEXT_HPP_ */
