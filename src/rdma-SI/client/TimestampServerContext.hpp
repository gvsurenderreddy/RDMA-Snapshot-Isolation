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
	struct ibv_mr *recv_mr;						// handler for the RECEIVE message
	struct ibv_mr *transaction_result_mr;
	struct ibv_mr *transaction_result_ack_mr;
	struct ibv_mr *read_timestamp_mr;			// not sure if it is needed
	struct ibv_mr *commit_timestamp_mr;			// not sure if it is needed

	// local memory buffers
	struct message::TimestampServerMemoryKeys recv_msg;	// buffer for the RECEIVE message
	Timestamp read_timestamp;
	Timestamp commit_timestamp;
	struct message::TransactionResult transaction_result;
	struct message::TransactionResult transaction_result_ack;

	// remote memory handlers
	struct ibv_mr peer_mr_read_timestamp;
	struct ibv_mr peer_mr_commit_timestamp;

	int register_memory();
	int destroy_context ();
};

#endif /* RDMA_NEWSI_CLIENT_TRXSERVERCONTEXT_HPP_ */
