/*
 *	rdma-client.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef RDMACLIENT_H_
#define RDMACLIENT_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../../config.hpp"
#include "../tpcw-tables/item_version.hpp"
#include "../tpcw-tables/orders_version.hpp"
#include "../tpcw-tables/order_line_version.hpp"
#include "../tpcw-tables/cc_xacts_version.hpp"
#include "../tpcw-tables/shopping_cart_line.hpp"
#include "../auxilary/timestamp_oracle.hpp"
#include "../auxilary/lock.hpp"


#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)


class RDMAClient{
private:
	static int		transaction_statement_number; 
	
	/* structure to exchange data which is needed to connect the QPs */
	struct CommunicationExchangeData
	{
		uint32_t	qp_num;		/* QP number */
		uint16_t	lid;			/* LID of the IB port */
		uint8_t		gid[16];		/* gid */
	} __attribute__ ((packed));


	struct Message {
		// We’ll use this to pass RDMA memory region (MR) keys between nodes and to signal that we’re done.
		// Note that we don't use this structure for RDMA operations
	
		struct ibv_mr mr_items;
		struct ibv_mr mr_orders;
		struct ibv_mr mr_order_line;
		struct ibv_mr mr_cc_xacts;
		struct ibv_mr mr_timestamp;
		struct ibv_mr mr_lock_items; 
	};
	
	struct ShoppingCart {
		ShoppingCartLine	cart_lines[ORDERLINE_PER_ORDER];
	};
	
	struct SharedContext
	{
		uint64_t	read_timestamp;		
		uint64_t	commit_timestamp;
	};
	
	static ShoppingCart		shopping_cart;
	static SharedContext	shared_context;	

	/* structure of Context */
	struct Context
	{
		char	server_address[50];
		int		tcp_port;
		int		ib_port;
		int		client_sockfd;							/* TCP socket file descriptor */
		
		struct	ibv_comp_channel *comp_channel;
	
		struct	ibv_device_attr device_attr;
		struct	ibv_port_attr port_attr;				/* IB port attributes */
		struct	CommunicationExchangeData remote_props;	/* values to connect to remote side */
		struct	ibv_context *ib_ctx;					/* device handle */
		struct	ibv_pd *pd;								/* PD handle */
		struct	ibv_cq *cq;								/* CQ handle */
		struct	ibv_qp *qp;								/* QP handle */
	
		// memory handler for buffers
		struct ibv_mr *recv_mr;			
		struct ibv_mr *local_items_mr;	
		struct ibv_mr *local_orders_mr;	
		struct ibv_mr *local_order_line_mr;	
		struct ibv_mr *local_cc_xacts_mr;	
		struct ibv_mr *local_read_timestamp_mr;	
		struct ibv_mr *local_commit_timestamp_mr;	
		struct ibv_mr *local_lock_items_mr;	
 
		struct ibv_mr peer_mr_items;
		struct ibv_mr peer_mr_orders;
		struct ibv_mr peer_mr_order_line;
		struct ibv_mr peer_mr_cc_xacts;
		struct ibv_mr peer_mr_timestamp;
		struct ibv_mr peer_mr_lock_items;
		
		// bufferes
		struct Message		*recv_msg;
		ItemVersion			*local_items_region;
		OrdersVersion		local_orders_region;
		OrderLineVersion	*local_order_line_region;
		CCXactsVersion		local_cc_xacts_region;
		TimestampOracle		local_read_timestamp_region;		
		TimestampOracle		local_commit_timestamp_region;
		uint64_t			*local_lock_items_region;
		
		int					fetch_block_num;	// shows which block of the remote region is being read
		
		ShoppingCartLine	*associated_cart_line;
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
	
	/******************************************************************************
	* Function: check_if_version_is_in_block
	*
	* Input
	* res pointer to Context structure
	*
	* Returns
	* either -1 or an index greater than or equal to 0.
	* if cannot find an item which satisfies the version in the buffer return -1
	* and in case it can find, it returns the index of that item
	*
	* Description
	* Cleanup and deallocate all Context used
	******************************************************************************/
	static int check_if_version_is_in_block(int version, ItemVersion *items_region, int size);
	
	static void fill_shopping_cart(Context *ctx);
	
	static void* get_item_info(Context *ctx);
	
	static void* acquire_item_lock(Context *ctx, uint64_t *expected_lock, uint64_t *new_lock);
	
	static void* revert_lock(Context *ctx, uint64_t *new_lock);
	
	static void* decrement_item_stock(Context *ctx);
	
	static void* release_lock(Context *ctx);
	
	static int start_transaction(Context *ctx);


public:
	
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