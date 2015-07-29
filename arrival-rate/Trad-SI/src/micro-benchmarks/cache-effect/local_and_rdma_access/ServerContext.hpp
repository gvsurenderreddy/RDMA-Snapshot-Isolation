/*
 *	ServerContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#ifndef SERVER_CONTEXT_H_
#define SERVER_CONTEXT_H_

#include "../../../util/BaseContext.hpp"
#include "../../../util/RDMACommon.hpp"
#include "MyStruct.hpp"

#include <iostream>

class ServerContext : public BaseContext {
public:
	std::string	server_address;	
	
	// memory handlers
	struct ibv_mr *local_mr;
	struct ibv_mr *send_message_mr;
	
	
	// memory buffers	
	struct MyStruct *local_buffer;
	struct ibv_mr	send_message_msg;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // SERVER_CONTEXT_H_