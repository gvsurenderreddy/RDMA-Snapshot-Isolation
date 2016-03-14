/*
 *	RDMAServer.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef RDMASERVER_H_
#define RDMASERVER_H_

#include "RDMAServerContext.hpp"
#include "../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class RDMAServer{
private:
	int	server_sockfd;		// Server's socket file descriptor
	int	tcp_port;
	int	ib_port;
	
	// memory buffers
	ItemVersion			*global_items_region;
	OrdersVersion		*global_orders_region;
	OrderLineVersion	*global_order_line_region;
	CCXactsVersion		*global_cc_xacts_region;
	Timestamp			*global_timestamp_region;
	uint64_t			*global_lock_items_region;

	
	int initialize_data_structures();
		
	int initialize_context(RDMAServerContext &ctx);
	
public:
	
	/******************************************************************************
	* Function: start_server
	*
	* Input
	* server_number (e.g. 0, 1, ...., Config.SERVER_CNT)
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the server. 
	*
	******************************************************************************/
	int start_server (int server_num);
	
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
	
	~RDMAServer ();
	
};
#endif /* RDMA_SERVER_H_ */