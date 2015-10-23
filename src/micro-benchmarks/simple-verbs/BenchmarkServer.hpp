/*
 *	BenchmarkServer.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef BENCHMARK_SERVER_H_
#define BENCHMARK_SERVER_H_

#include "ServerContext.hpp"
#include "../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class BenchmarkServer{
private:
	const uint32_t client_cnt_;
	int	server_sockfd;		// Server's socket file descriptor
	uint16_t	tcp_port;
	uint8_t		ib_port;
	
	// memory buffers
	char local_buffer[benchmark_config::SERVER_REGION_SIZE];
	
	static void* handle_client(void *param);
	
public:
	BenchmarkServer(uint32_t client_cnt);

	int start_server ();
	static void usage (const char *argv0);
	
	~BenchmarkServer();
	
};
#endif /* BENCHMARK_SERVER_RDMA_H_ */
