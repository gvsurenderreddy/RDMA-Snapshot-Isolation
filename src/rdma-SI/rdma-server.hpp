/*
 *	rdma-server.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders_version.hpp"
#include "../tpcw-tables/order_line_version.hpp"
#include "../tpcw-tables/cc_xacts_version.hpp"
#include "../auxilary/timestamp_oracle.hpp"
#include "../auxilary/lock.hpp"


#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero).");  } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

class Server{
private:
	static ItemVersion		*items_region;
	static OrdersVersion	*orders_region;
	static OrderLineVersion	*order_line_region;
	static CCXactsVersion	*cc_xacts_region;
	static TimestampOracle	*timestamp_region;
	static uint64_t			*lock_items_region;
	
	static int				server_sockfd;		// Server's socket file descriptor

	/* structure to exchange data which is needed to connect the QPs */
	struct CommunicationExchangeData
	{
		uint32_t qp_num;		/* QP number */
		uint16_t lid;			/* LID of the IB port */
		uint8_t gid[16];		/* gid */
	} __attribute__ ((packed));

	/* structure of system resources */

	struct Message {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes and to signal
		// that we’re done. Note that we don't use this structure for RDMA operations
		struct ibv_mr mr_items;
		struct ibv_mr mr_orders;
		struct ibv_mr mr_order_line;
		struct ibv_mr mr_cc_xacts;
		struct ibv_mr mr_timestamp;
		struct ibv_mr mr_lock_items; 
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
	
		int client_sockfd;			/* TCP socket file descriptor */
	
		struct ibv_mr *send_mr;
		struct ibv_mr *mr_items;
		struct ibv_mr *mr_orders;		
		struct ibv_mr *mr_order_line;
		struct ibv_mr *mr_cc_xacts;
		struct ibv_mr *mr_timestamp;
		struct ibv_mr *mr_lock_items;
	
		struct Message *send_msg;
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

