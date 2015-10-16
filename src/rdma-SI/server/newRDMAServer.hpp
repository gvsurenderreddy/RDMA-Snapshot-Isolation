/*
 *	RDMAServer.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMASERVER_H_
#define RDMASERVER_H_

#include "../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "newRDMAServerContext.hpp"


class RDMAServer{
private:
	int	server_sockfd;		// Server's socket file descriptor
	uint16_t	tcp_port;
	uint8_t	ib_port;
	
	// memory buffers
	ItemVersion			*global_items_region;
	OrdersVersion		*global_orders_region;
	OrderLineVersion	*global_order_line_region;
	CCXactsVersion		*global_cc_xacts_region;
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
	
	~RDMAServer ();
	
};
#endif /* RDMA_SERVER_H_ */
