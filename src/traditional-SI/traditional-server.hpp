/*
 *	traditional-server.hpp
 *
 *	Created on: 9.Feb.2015
 *	Author: erfanz
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../../config.hpp"
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders_version.hpp"
#include "../tpcw-tables/order_line_version.hpp"
#include "../tpcw-tables/cc_xacts_version.hpp"
#include "../auxilary/timestamp_oracle.hpp"
#include "../auxilary/lock.hpp"
#include <mutex>


#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero).");  } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

class Server{
private:
	// Buffers
	static ItemVersion		*items_region;
	static OrdersVersion	*orders_region;
	static OrderLineVersion	*order_line_region;
	static CCXactsVersion	*cc_xacts_region;
	static TimestampOracle	timestamp_region;
	
	// Locks
	static std::mutex 		timestamp_mutex;
	static std::mutex 		item_lock[MAX_ITEM_CNT];
	
	static int				server_sockfd;		// Server's socket file descriptor
	
	
	/* structure to exchange data which is needed to connect the QPs */
	struct CommunicationExchangeData
	{
		uint32_t qp_num;		/* QP number */
		uint16_t lid;			/* LID of the IB port */
		uint8_t gid[16];		/* gid */
	} __attribute__ ((packed));

	
	struct ItemInfoRequest {
		int I_ID;
	};
	
	struct ItemInfoResponse {
		Item item;
	};
	
	struct CommitRequest {
		Orders		order;
		OrderLine	orderlines[ORDERLINE_PER_ORDER];
		CCXacts		ccxact;
	};
	
	struct CommitResponse {
		enum CommitOutcome {
			COMMITTED,
			ABORTED
		} commit_outcome;
		char	reason[30];
	};

	struct Context
	{
		struct ibv_comp_channel *comp_channel;
		struct ibv_port_attr port_attr;	/* IB port attributes */
		struct CommunicationExchangeData remote_props;	/* values to connect to remote side */
		struct ibv_context *ib_ctx;	/* device handle */
		struct ibv_pd *pd;		/* PD handle */
		struct ibv_cq *cq;		/* CQ handle */
		struct ibv_qp *qp;		/* QP handle */
	
		int transaction_statement_number; 
	
		int client_sockfd;			/* TCP socket file descriptor */
	
		struct ibv_mr *item_info_req_mr;
		struct ibv_mr *item_info_res_mr;
		struct ibv_mr *commit_req_mr;
		struct ibv_mr *commit_res_mr;
		
		struct ItemInfoRequest	*item_info_request;
		struct ItemInfoResponse	*item_info_response;
		struct CommitRequest	*commit_request;
		struct CommitResponse	*commit_response;
	};


	static int initialize_data_structures();
	static int load_tables_from_files();


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
	static int create_context(struct Context *ctx);

	static void* handle_client(void *param);
	
	static int build_connection(Context *ctx);
	static int register_memory(Context *ctx);

	/******************************************************************************
	* Function: connect_qp
	*
	* Input
	* res pointer to resources structure
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
	static int connect_qp (struct Context *ctx);

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
	static int destroy_context (struct Context *ctx);
	
	static int destroy_resources ();

	static void die(const char *reason);
	
	
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
#endif /* SERVER_H_ */