/*
 *	BenchmarkServer.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#include "BenchmarkServerRDMA.hpp"
#include "../benchmark-config.hpp"
#include "../../../config.hpp"
#include "../../util/utils.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

BenchmarkServerRDMA::~BenchmarkServerRDMA () {
	delete[](local_buffer);
	close(server_sockfd);
}

int BenchmarkServerRDMA::initialize_data_structures(){
	local_buffer = new char[SERVER_REGION_SIZE];
	for (int i = 0; i < SERVER_REGION_SIZE; i++)
		local_buffer[i] = 0;
	return 0;
}

int BenchmarkServerRDMA::initialize_context(ServerContext &ctx) {
	ctx.ib_port				= ib_port;
	ctx.local_buffer		= local_buffer;
	return 0;
}

void BenchmarkServerRDMA::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " <i = server_num>" << std::endl;
	std::cout << "starts a server and waits for connection on port Config.TCP_PORT[i]" << std::endl;
	std::cout << "(valid range of i: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
}

void* BenchmarkServerRDMA::handle_client(void *param) {
	ServerContext *ctx = (ServerContext *) param;
	char temp_char;
	
	DEBUG_COUT("[in handle client]");
	
	TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->recv_data_mr, (uintptr_t)&ctx->recv_data_msg, sizeof(int)));
	TEST_NZ (sock_sync_data (ctx->sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	
	DEBUG_COUT("[Synved with client]");
	
	
	int iteration = 0;
	while (iteration < OPERATIONS_CNT) {
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));
		DEBUG_COUT("[Recv] request from client");
		
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->recv_data_mr, (uintptr_t)&ctx->recv_data_msg, sizeof(int)));	// for 
		DEBUG_COUT("[Info] receive posted to the queue");
		
		if (iteration % 1000 == 0) {
			TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->send_data_mr, (uintptr_t)ctx->send_data_msg, BUFFER_SIZE * sizeof(char), true));
			DEBUG_COUT("[Sent] response to client");
		
			TEST_NZ (RDMACommon::poll_completion(ctx->cq));	// for SEND
			DEBUG_COUT("[Info] completion received");
		
		}
		else {
			TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->send_data_mr, (uintptr_t)ctx->send_data_msg, BUFFER_SIZE * sizeof(char), false));
			DEBUG_COUT("[Sent] response to client (without completion)");
		}		
		iteration++;
	}
	DEBUG_COUT("[Sent] buffer info to client");
}


int BenchmarkServerRDMA::start_server (int server_num) {	
	tcp_port	= TCP_PORT[server_num];
	ib_port		= IB_PORT[server_num];
	ServerContext ctx[CLIENTS_CNT];
	pthread_t master_threads[CLIENTS_CNT];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	char temp_char;
	
	
	TEST_NZ(initialize_data_structures());

	std::cout << "[Info] Server " << server_num << " is waiting for " << CLIENTS_CNT
		<< " client(s) on tcp port: " << tcp_port << ", ib port: " << ib_port << std::endl;
	
	// Open Socket
	server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		std::cerr << "Error opening socket" << std::endl;
		return -1;
	}
	
	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(tcp_port);
	TEST_NZ(bind(server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	

	// listen				  
	TEST_NZ(listen (server_sockfd, CLIENTS_CNT));
	
	
	// accept connections
	for (int i = 0; i < CLIENTS_CNT; i++){
		initialize_context(ctx[i]);
		ctx[i].sockfd  = accept (server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (ctx[i].sockfd < 0){
			std::cerr << "ERROR on accept" << std::endl;
			return -1;
		}
		std::cout << "[Conn] Received client #" << i << " on socket " << ctx[i].sockfd << std::endl;
	
		// create all resources
		TEST_NZ (ctx[i].create_context());
		DEBUG_COUT("[Info] Context for client " << i << " created");
		
		// connect the QPs
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));
		DEBUG_COUT("[Conn] QPed to client " << i);
	
		// prepare server buffer with read message
		memcpy(&(ctx[i].send_message_msg.peer_mr), ctx[i].local_mr, sizeof(struct ibv_mr));
	}
	
	for (int i = 0; i < CLIENTS_CNT; i++){
		// send memory locations using SEND 
		TEST_NZ (RDMACommon::post_SEND (ctx[i].qp, ctx[i].send_message_mr, (uintptr_t)&ctx[i].send_message_msg, sizeof(struct MemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));
		DEBUG_COUT("[Sent] buffer info to client " << i);
	}
	
	
	
	// Server waits for the client to muck with its memory
	
	
	/*************** THIS IS FOR SEND
	// accept connections
	for (int i = 0; i < CLIENTS_CNT; i++){
		initialize_context(ctx[i]);
		ctx[i].sockfd  = accept (server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (ctx[i].sockfd < 0){
			std::cerr << "ERROR on accept" << std::endl;
			return -1;
		}
		std::cout << "[Conn] Received client #" << i << " on socket " << ctx[i].sockfd << std::endl;
	
		// create all resources
		TEST_NZ (ctx[i].create_context());
		DEBUG_COUT("[Info] Context for client " << i << " created");
		
		// connect the QPs
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));
		DEBUG_COUT("[Conn] QPed to client " << i);
	
		pthread_create(&master_threads[i], NULL, BenchmarkServer::handle_client, &ctx[i]);
	}
	std::cout << "[Info] Established connection to all " << CLIENTS_CNT << " client(s)." << std::endl; 
	
	//wait for handlers to finish
	for (int i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	*/

	for (int i = 0; i < CLIENTS_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
		DEBUG_COUT("[Conn] Client " << i << " notified it's finished");
		TEST_NZ (ctx[i].destroy_context());
		std::cout << "[Info] Destroying client " << i << " resources" << std::endl;
	}
	

	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;	
}

int main (int argc, char *argv[]) {
	if (argc != 2) {
		BenchmarkServerRDMA::usage(argv[0]);
		return 1;
	}
	BenchmarkServerRDMA server;
	server.start_server(atoi(argv[1]));
	return 0;
}