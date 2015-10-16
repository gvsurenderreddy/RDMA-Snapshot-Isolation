/*
 *	Benchmark.cpp
 *
 *	Created on: 26.March.2015
 *	Author: Erfan Zamanian
 */

#include "Client.hpp"
#include "../../../util/utils.hpp"
#include "../../../../config.hpp"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <time.h>	// for struct timespec
#include <sys/resource.h>	// getrusage()


void Client::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

int Client::start_benchmark(ClientContext &ctx) {
	
	DEBUG_COUT ("[Info] Benchmark now gets started");
	
	// int item_number;
	// int offset;
	// uint64_t *remote_address;
	bool signaled;
	
	
	int iteration = 0;
	
	while (iteration < 10000) {
		if (iteration % 1000 == 0)
			signaled = true;
		else
			signaled = false;

		// item_number		= rand() % ARRAY_SIZE;
		// offset			= item_number * sizeof(struct MyStruct);
		// remote_address	= (uint64_t *)(offset + ((uint64_t)ctx.peer_mr.addr));

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
		ctx.qp,
		ctx.local_mr,
		(uint64_t)ctx.local,
		&(ctx.peer_mr),
		//(uint64_t)remote_address,
		//(uint32_t)sizeof(struct MyStruct),
		(uint64_t)ctx.peer_mr.addr,
		(uint32_t)ARRAY_SIZE * sizeof(struct MyStruct),
		signaled));
		if (signaled)
			TEST_NZ (RDMACommon::poll_completion (ctx.cq));		// completion for WRITE

		// DEBUG_COUT ("[READ] to buffer #" << item_number);

		iteration++;
	}
	
	// long int sum = 0;
// 	for (iteration = 0; iteration < 40; iteration++) {
// 		sum = 0;
// 		for (int item_number = 0; item_number < ARRAY_SIZE; item_number++){
// 			// i++;
// 			// if (i % 1000 == 0)
// 			// 	signaled = true;
// 			// else
// 			// 	signaled = false;
//
// 			offset			= item_number * sizeof(struct MyStruct);
// 			remote_address	= (uint64_t *)(offset + ((uint64_t)ctx.peer_mr.addr));
//
// 			TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
// 			ctx.qp,
// 			ctx.local_mr,
// 			(uint64_t)&ctx.local,
// 			&(ctx.peer_mr),
// 			(uint64_t)remote_address,
// 			(uint32_t)sizeof(struct MyStruct),
// 			true));
// 			//if (signaled)
// 			TEST_NZ (RDMACommon::poll_completion (ctx.cq));		// completion for WRITE
// 			sum += ctx.local.pad[0];
// 			DEBUG_COUT ("[READ] to buffer #" << item_number);
// 		}
// 		// std::cout << "Sum: " << sum << std::endl;
// 	}
	
	//std::cout << "[Stat] Avg latency(u sec):   	" << latency_in_micro << std::endl; 
	//std::cout << "[Stat] MegaByte per Sec:   	" << mega_byte_per_sec <<  std::endl;
	std::cout << "Client is done with the server" << std::endl;
	return 0;
}

int Client::start_client () {	
	ClientContext ctx;
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed

		
	// then, client connext to servers
	ctx.server_address = "";
	ctx.server_address += SERVER_ADDR[0];
	ctx.ib_port		  = IB_PORT[0];
		
		
	// Connect to servers
	TEST_NZ (establish_tcp_connection(ctx.server_address, TCP_PORT[0], &ctx.sockfd));
	DEBUG_COUT("[Conn] Connection established to server");
	
	TEST_NZ (ctx.create_context());
		
	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(ctx.qp, ctx.recv_memory_mr, (uintptr_t)&ctx.recv_memory_msg, sizeof(struct ibv_mr));
		
	TEST_NZ (RDMACommon::connect_qp (&(ctx.qp), ctx.ib_port, ctx.port_attr.lid, ctx.sockfd));
	
	TEST_NZ(RDMACommon::poll_completion(ctx.cq));
	DEBUG_COUT("[Recv] buffers info from server");
		
	// after receiving the message from the server, let's store its addresses in the context
	memcpy(&ctx.peer_mr,	&ctx.recv_memory_msg,	sizeof(struct ibv_mr));
		
	DEBUG_COUT ("[Info] Successfully connected to all servers");
	
	start_benchmark(ctx);
	
	DEBUG_COUT("[Info] Client is done, and is ready to destroy its resources!");
	
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	DEBUG_COUT("[Conn] Notified server it's done");
	TEST_NZ ( ctx.destroy_context());
	return 0;
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		Client::usage(argv[0]);
		return 1;
	}
	Client client;
	client.start_client();
	return 0;
}