/*
 *	timestamp-server.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TIMESTAMPSERVER_H_
#define TIMESTAMPSERVER_H_

#include "../../../config.hpp"
#include "../../auxilary/timestamp.hpp"
#include "../shared/newRDMAMessage.hpp"
#include "../../util/BaseContext.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <mutex>
#include <vector>
#include <queue>          // std::priority_queue
#include <utility>      // std::pair, std::make_pair
#include <mutex>
#include <condition_variable>

typedef message::TransactionResult::Result Result;
typedef std::pair <Timestamp,  Result> TimestampResultPair;


struct QueueEntryComparator  {
	bool operator() (TimestampResultPair pair1, TimestampResultPair pair2) {
		return pair1.first > pair2.first;
	}
};


class TimestampServer{
private:
	int	server_sockfd;		// Server's socket file descriptor
	std::priority_queue<TimestampResultPair, std::vector<TimestampResultPair>, QueueEntryComparator> finishedTrxs;

	double avg_queue_size = 0;
	int sample_size = 0;

	std::mutex mu;
	std::condition_variable cond;

	Timestamp	read_timestamp;
	Timestamp	commit_timestamp;

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
		struct ibv_mr *send_mr;		// for the SEND message
		struct ibv_mr *mr_transaction_result;
		struct ibv_mr *mr_transaction_result_ack;
		struct ibv_mr *mr_read_timestamp;
		struct ibv_mr *mr_commit_timestamp;


		// memory buffers
		struct message::TimestampServerMemoryKeys send_msg;
		struct message::TransactionResult transaction_result;
		struct message::TransactionResult transaction_result_ack;

	};

	int initialize_data_structures();

	int registerGlobalMemoryRegions();



	/******************************************************************************
	* Function: create_context
	*
	* Input
	* pointer to the resources structure for the client
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	*
	* This function creates and allocates all necessary system resources. These
	* are stored in res (the input argument).
	*****************************************************************************/
	int	create_context(struct WorkerContext *ctx);

	void handle_client(struct WorkerContext *ctx);
	
	int	register_memory(struct WorkerContext *ctx);
		
	std::string	get_full_desc(struct WorkerContext *ctx);
	
	int appendTidToUnresolved(Timestamp &ts, Result &result);

	int cleanUpFinishedTransactions();




	/******************************************************************************
	* Function: destroy_resources
	*
	* Input
	* res pointer to resources structure
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	* Cleanup and deallocate all resources used
	******************************************************************************/
	int destroy_client_context (struct WorkerContext *ctx);
	
	int destroy_resources ();
	
public:
	
	/******************************************************************************
	* Function: start_server
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the server. 
	*
	******************************************************************************/
	int start_server ();
	
	/******************************************************************************
	* Function: usage
	*
	* Input
	* argv0 command line arguments
	*
	* Output
	* none
	*
	* Returns
	* none
	*
	* Description
	* print a description of command line syntax
	******************************************************************************/
	static void usage (const char *argv0);
};
#endif /* TIMESTAMPSERVER_H_ */
