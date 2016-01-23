/*
 *	timestamp-server.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TIMESTAMPSERVER_H_
#define TIMESTAMPSERVER_H_

#include "../../../config.hpp"
#include "../shared/newRDMAMessage.hpp"
#include "../../util/BaseContext.hpp"
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include "../../basic-types/timestamp.hpp"
#include "../../basic-types/PrimitiveTypes.hpp"


typedef message::TransactionResult::Result Result;
typedef std::pair <Timestamp,  Result> TimestampResultPair;


struct QueueEntryComparator  {
	bool operator() (TimestampResultPair pair1, TimestampResultPair pair2) {
		return pair1.first > pair2.first;
	}
};

class TimestampServer{
private:
	const size_t	clients_cnt_;	// coming from the command line.
	int				server_sockfd_;	// Server's socket file descriptor

	//uint8_t 	*finished_trxs_hash_;
	//Timestamp	read_trx_;

	primitive::timestamp_t *last_trx_timestamp_per_client_;

	struct WorkerContext  {
		struct	ibv_device_attr device_attr;
		struct	ibv_port_attr port_attr;				/* IB port attributes */
		struct	ibv_context *ib_ctx;					/* device handle */
		struct	ibv_pd *pd;								/* PD handle */
		struct	ibv_cq *send_cq;						/* CQ handle */
		struct	ibv_cq *recv_cq;						/* CQ handle */
		struct	ibv_qp *qp;								/* QP handle */
		struct	ibv_comp_channel *send_comp_channel;	/* CQ channel */
		struct	ibv_comp_channel *recv_comp_channel;	/* CQ channel */


		uint8_t	ib_port;
		int 	sockfd = -1;
		std::string client_ip;
		int	client_port;
		int trx_num;

		// memory handlers
		struct ibv_mr *mr_send;		// for the SEND message
		//struct ibv_mr *mr_finished_trxs_hash;
		//struct ibv_mr *mr_read_trx;
		struct ibv_mr *mr_last_trx_timestamp_per_client;

		// memory buffers
		struct message::TimestampServerMemoryKeys send_msg;
	};

	int registerGlobalMemoryRegions();

	int	create_context(struct WorkerContext &ctx);

	void handle_client(struct WorkerContext &ctx, primitive::client_id_t client_id);
	
	int	register_memory(struct WorkerContext &ctx);
		
	std::string	get_full_desc(struct WorkerContext &ctx);
	
	int cleanUpFinishedTransactions();

	int destroy_client_context (struct WorkerContext &ctx);
	
	int destroy_resources ();
	
public:
	
	TimestampServer(size_t clients_cnt);

	int start_server ();
	
	static void usage (const char *argv0);
};
#endif /* TIMESTAMPSERVER_H_ */
