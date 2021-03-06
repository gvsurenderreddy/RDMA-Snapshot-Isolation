/*
 *	CoordinatorContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#ifndef COORDINATOR_CONTEXT_H_
#define COORDINATOR_CONTEXT_H_

#include "../../../../util/BaseContext.hpp"
#include "../../../../util/RDMACommon.hpp"
#include "../../MemoryKeys.hpp"
#include <iostream>

class CoordinatorContext : public BaseContext{
public:
	std::string	server_address;
		
	
	// memory handler
	struct ibv_mr *recv_memory_mr;			
	struct ibv_mr *send_data_mr;	
	struct ibv_mr *recv_data_mr;	
	
	// memory bufferes
	struct MemoryKeys	recv_memory_msg;
	int send_data_msg;
	int recv_data_msg;
	
	// remote memory handlers
	struct ibv_mr peer_data_mr;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // COORDINATOR_CONTEXT_H_