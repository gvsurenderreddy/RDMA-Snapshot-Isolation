/*
 *	client.hpp
 *
 *	Created on: 25.01.2015
 *	Author: erfanz
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders_version.hpp"
#include "../tpcw-tables/cc_xacts_version.hpp"
#include "../timestamp/timestamp_oracle.hpp"

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)


class Client{
private:
	/* structure to exchange data which is needed to connect the QPs */
	struct CommunicationExchangeData
	{
		uint32_t qp_num;		/* QP number */
		uint16_t lid;			/* LID of the IB port */
		uint8_t gid[16];		/* gid */
	} __attribute__ ((packed));


	struct Message {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes and to signal that we’re done.
		// Note that we don't use this structure for RDMA operations
	
		struct ibv_mr rdma_mr_items;
		struct ibv_mr rdma_mr_orders;
		struct ibv_mr rdma_mr_cc_xacts;
		struct ibv_mr rdma_mr_timestamp_oracle; 
	};

	/* structure of Context */
	struct Context
	{
		struct ibv_comp_channel *comp_channel;
	
		struct ibv_device_attr device_attr;
		/* Device attributes */
		struct ibv_port_attr port_attr;	/* IB port attributes */
		struct CommunicationExchangeData remote_props;	/* values to connect to remote side */
		struct ibv_context *ib_ctx;	/* device handle */
		struct ibv_pd *pd;		/* PD handle */
		struct ibv_cq *cq;		/* CQ handle */
		struct ibv_qp *qp;		/* QP handle */
	
	
		int client_sockfd;			/* TCP socket file descriptor */
		pthread_t cq_poller_thread;		/* the thread which polls from the completion queue */
	
	
		int fetch_block_num;	// shows which block of the remote region is being read
		int transaction_statement_number; 
		int item_id;

		struct ibv_mr *recv_mr;			// infiniband memory handler for the receiving message
		struct ibv_mr *local_rdma_items_mr;	// infiniband memory handler for the RDMA memory region for items
		struct ibv_mr *local_rdma_orders_mr;	// infiniband memory handler for the RDMA memory region for orders
		struct ibv_mr *local_rdma_cc_xacts_mr;	// infiniband memory handler for the RDMA memory region for cc xacts
		struct ibv_mr *local_rdma_timestamp_oracle_mr;	// the infiniband memory handler for the RDMA for timestamp oracle
 
		struct ibv_mr peer_mr_items;
		struct ibv_mr peer_mr_orders;
		struct ibv_mr peer_mr_cc_xacts;
		struct ibv_mr peer_mr_timestamp_oracle;
 
		struct Message *recv_msg;	// the memory region for the receiving message
		ItemVersion *local_rdma_items_region;
		OrdersVersion *local_rdma_orders_region;
		CCXactsVersion *local_rdma_cc_xacts_region;
		TimestampOracle *local_rdma_timestamp_region;	// it will have two elements, one for read timestamp, one for commit timestamp
	
		enum {
			POINTER_READING,
			DATA_READING
		} read_state;
	
		enum {
			BEGIN_TRANSACTION,
			ACQUIRING_READ_TIMESTAMP,
			ACQUIRING_COMMIT_TIMESTAMP,
			LOCKING_ITEM,
			FETCHING_ITEM_POINTER,
			FETCHING_ITEM_INFORMATION,
			ADDING_RECORD_TO_ORDERS,
			ADDING_RECORD_TO_CCXACTS,
			UPDATING_ITEM_TABLE,
			UNLOCKING_TABLES,
			END_TRANSACTION
		} state;
	};



	/******************************************************************************
	* Function: create_context
	*
	* Input
	* pointer to the Context structure for the client
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	*
	* This function creates and allocates all necessary system Context. These
	* are stored in res (the input argument).
	*****************************************************************************/
	static int create_context(struct Context *res);

	static int build_connection(Context *res);
	static int register_memory(Context *res);


	/******************************************************************************
	* Function: connect_qp
	*
	* Input
	* res pointer to Context structure
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, error code on failure
	*
	* Description
	* Connect the QP. Transition the server side to RTR, sender side to RTS
	******************************************************************************/
	static int connect_qp (struct Context *res);


	/******************************************************************************
	* Function: destroy_context
	*
	* Input
	* res pointer to Context structure
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	* Cleanup and deallocate all Context used
	******************************************************************************/
	static int destroy_context (struct Context *ctx);

	static void die(const char *reason);

	

public:
	
	static char* server_name;
	
	/******************************************************************************
	* Function: start_client
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the client. 
	*
	******************************************************************************/
	int start_client ();
	
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

#endif /* CLIENT_H_ */
