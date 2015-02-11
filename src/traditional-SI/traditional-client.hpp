/*
 *	rdma-client.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../../config.hpp"
#include "../tpcw-tables/item.hpp"
#include "../tpcw-tables/orders.hpp"
#include "../tpcw-tables/order_line.hpp"
#include "../tpcw-tables/cc_xacts.hpp"


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
	
		int fetch_block_num;	// shows which block of the remote region is being read
		int transaction_statement_number; 

		
		struct ibv_mr *item_info_req_mr;
		struct ibv_mr *item_info_res_mr;
		struct ibv_mr *commit_req_mr;
		struct ibv_mr *commit_res_mr;
		
		struct ItemInfoRequest	*item_info_request;
		struct ItemInfoResponse	*item_info_response;
		struct CommitRequest	*commit_request;
		struct CommitResponse	*commit_response;
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
	static int create_context(struct Context *ctx);

	static int build_connection(Context *ctx);
	static int register_memory(Context *ctx);


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
	static int connect_qp (struct Context *ctx);


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
	
	
	static int start_transaction(Context *ctx);
	

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
