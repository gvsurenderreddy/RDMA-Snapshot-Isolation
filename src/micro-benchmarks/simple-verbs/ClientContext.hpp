/*
 *	ClientContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#ifndef CLIENT_CONTEXT_H_
#define CLIENT_CONTEXT_H_

#include "../../util/BaseContext.hpp"
#include "../../util/RDMACommon.hpp"
#include "../benchmark-config.hpp"
#include "MemoryKeys.hpp"
#include <iostream>

class ClientContext : public BaseContext{
public:
	std::string	server_address;
		
	
	// memory handler
	struct ibv_mr *recv_memory_mr;			
	struct ibv_mr *send_data_mr;	
	struct ibv_mr *recv_data_mr;	
	
	// memory buffers
	struct MemoryKeys	recv_memory_msg;
	uint64_t send_data_msg[benchmark_config::BUFFER_WORDS];
	uint64_t recv_data_msg[benchmark_config::BUFFER_WORDS];
	//uint64_t send_data_msg;
	//uint64_t recv_data_msg;

	
	// remote memory handlers
	struct ibv_mr peer_data_mr;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // CLIENT_CONTEXT_H_
