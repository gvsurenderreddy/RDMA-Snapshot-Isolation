/*
 *	ServerContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#ifndef SERVER_CONTEXT_H_
#define SERVER_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../benchmark-config.hpp"
#include "MemoryKeys.hpp"
#include "../../util/RDMACommon.hpp"
#include <iostream>

class ServerContext : public BaseContext {
public:
	std::string	server_address;	
	
	// memory handlers
	struct ibv_mr *send_message_mr;
	struct ibv_mr *global_buffer_mr;
	
	struct ibv_mr *send_data_mr;
	struct ibv_mr *recv_data_mr;
	

	// memory buffers
	struct MemoryKeys send_message_msg;
	
	char *global_buffer;
	
	char send_data_msg[benchmark_config::SERVER_REGION_SIZE];
	char recv_data_msg[benchmark_config::SERVER_REGION_SIZE];
	
	
	int register_memory();
	int destroy_context ();
};
#endif // SERVER_CONTEXT_H_
