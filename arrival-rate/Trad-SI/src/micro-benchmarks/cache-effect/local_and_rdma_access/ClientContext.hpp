/*
 *	ClientContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#ifndef CLIENT_CONTEXT_H_
#define CLIENT_CONTEXT_H_

#include "../../../util/BaseContext.hpp"
#include "../../../util/RDMACommon.hpp"
#include "MyStruct.hpp"
#include <iostream>

class ClientContext : public BaseContext{
public:
	std::string	server_address;
		
	
	// memory handler
	struct ibv_mr *local_mr;
	struct ibv_mr *recv_memory_mr;	
	
	// memory bufferes
	struct ibv_mr	recv_memory_msg;
	struct MyStruct local[ARRAY_SIZE];
	
	struct ibv_mr	peer_mr;
	
	int register_memory();
	int destroy_context ();
};
#endif // CLIENT_CONTEXT_H_