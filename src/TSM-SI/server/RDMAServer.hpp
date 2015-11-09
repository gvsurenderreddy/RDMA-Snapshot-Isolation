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

#include "RDMAServerContext.hpp"


class RDMAServer{
private:
	int	server_sockfd_;		// Server's socket file descriptor
	uint16_t	tcp_port_;
	uint8_t	ib_port_;
	const uint32_t clients_cnt_;
	const uint32_t server_num_;

	
	// memory buffers
	ItemVersion			*global_items_region;
	OrdersVersion		*global_orders_region;
	OrderLineVersion	*global_order_line_region;
	CCXactsVersion		*global_cc_xacts_region;
	uint64_t			*global_lock_items_region;
	
	int initialize_data_structures();
		
	int initialize_context(RDMAServerContext &ctx);
	
public:
	RDMAServer(uint32_t server_num, uint32_t clients_cnt);

	int start_server();
	
	~RDMAServer ();
	
};
#endif /* RDMA_SERVER_H_ */
