/*
 *	BenchmarkServerRDMA.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef BENCHMARK_SERVER_RMDA_H_
#define BENCHMARK_SERVER_RDMA_H_

#include "ServerContext.hpp"
#include "../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class BenchmarkServerRDMA{
private:
	int	server_sockfd;		// Server's socket file descriptor
	int	tcp_port;
	int	ib_port;
	
	// memory buffers
	char *local_buffer;
	
	int initialize_data_structures();
	int initialize_context(ServerContext &ctx);
	
	static void* handle_client(void *param);
	
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
	
	~BenchmarkServerRDMA ();
	
};
#endif /* BENCHMARK_SERVER_RDMA_H_ */