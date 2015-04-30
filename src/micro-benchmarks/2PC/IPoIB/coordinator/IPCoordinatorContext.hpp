/*
 *	IPCoordinatorContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#ifndef IP_COORDINATOR_CONTEXT_H_
#define IP_COORDINATOR_CONTEXT_H_

#include "../../../../util/BaseContext.hpp"
#include "../../../../util/RDMACommon.hpp"
#include <iostream>

class IPCoordinatorContext : public BaseContext{
public:
	std::string	server_address;
			
	// memory bufferes
	int send_data_msg;
	int recv_data_msg;
	
	// remote memory handlers
	struct ibv_mr peer_data_mr;
	
	
	int create_context ();
	int register_memory ();
	int destroy_context ();
};
#endif // IP_COORDINATOR_CONTEXT_H_