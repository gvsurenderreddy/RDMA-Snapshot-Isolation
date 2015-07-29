/*
 *	Server.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "Server.hpp"
#include "../../../../config.hpp"
#include "../../../util/utils.hpp"
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
#include <algorithm>    // std::shuffle
#include <vector>       // std::vector
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <iterator> 	// for ostream_iterator
// #include "papi.h"



Server::~Server () {
	delete[](global_buffer);
	close(server_sockfd);
}

int Server::initialize_data_structures(){
	global_buffer = new MyStruct[ARRAY_SIZE];
	//make_sequential_linked_list(global_buffer);
	make_random_linked_list(global_buffer);
	return 0;
}

int Server::initialize_context(ServerContext &ctx) {
	ctx.local_buffer	= global_buffer; 
	ctx.ib_port			= ib_port;
	return 0;
}

void Server::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << std::endl;
	std::cout << "starts a server and waits for connection on port Config.TCP_PORT" << std::endl;
}

void Server::make_sequential_linked_list(struct MyStruct *myS){	
	for (int i=0; i < ARRAY_SIZE - 1; i++){
		myS[i].n = &myS[i + 1];
		for (int j = 0; j < NPAD; j++)
			myS[i].pad[j] = (long int) i;
	}
	//myS[ARRAY_SIZE - 1].n = NULL;
	myS[ARRAY_SIZE - 1].n = &myS[0];
	for (int j = 0; j < NPAD; j++)
		myS[ARRAY_SIZE - 1].pad[j] = (long int) (ARRAY_SIZE - 1);
}

void Server::make_random_linked_list(struct MyStruct *myS){
	std::vector<int> seq;
	for (int i = 1; i < ARRAY_SIZE; i++)
		seq.push_back(i);

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle (seq.begin(), seq.end(), std::default_random_engine(seed));

	//std::copy(sequence.begin(), sequence.end(), std::ostream_iterator<int>(std::cout, " "));

	// First, we link myS[0] to the first element of the shuffled sequence (this way, myS[0] is always the beginning of the list)
	myS[0].n = &myS[seq[0]];
	for (int j = 0; j < NPAD; j++)
		myS[0].pad[j] = (long int) 0;

	for(unsigned int i = 0; i < seq.size() - 1; i++) {
		myS[seq[i]].n = &myS[seq[i + 1]];
		for (int j = 0; j < NPAD; j++)
			myS[seq[i]].pad[j] = (long int) seq[i];
	}

	//myS[seq.back()].n = NULL;
	myS[seq.back()].n = &myS[0];
	for (int j = 0; j < NPAD; j++)
		myS[seq.back()].pad[j] = (long int) seq.back();
}

int Server::fill_cache() {
	// Filling the cache
	// MyStruct *current = &global_buffer[0];
	// while ((current = current->n) != NULL)
	// 	current->pad[0] += 1;
	for (int i = 0; i < 1000; i++) {
		for (int j =0 ; j < ARRAY_SIZE; j++)
			global_buffer[j].pad[0] += 0.001;
	}

	DEBUG_COUT("[INFO] Cached initialized");
	return 0;
}

int Server::start_benchmark(){
	//int traverse_cnt;
	// int events[1] = {PAPI_L1_DCM}, ret;
	// long long values[1];
	
	// if (PAPI_num_counters() < 1) {
	// 	fprintf(stderr, "No hardware counters here, or PAPI not supported.\n");
	// 	return -1;
	// }

	TEST_NZ(fill_cache());
	
	
	// sleep for some time to make sure that client is running 
	sleep(10);

	
	
	// ************************************************	
	// Now, server performs a sequential scan
	
	//MyStruct *current = &global_buffer[0];	
	
	// double new_array[200000];
	// for (int i = 0; i < 200000; i++)
	// 	new_array[(int)rand() % 200000] = rand();
	
	
	
	// For CPU usage in terms of clocks (ticks)
	unsigned long long cpu_checkpoint_start = rdtscp();
	// ALTER: if ((ret = PAPI_start_counters(events, 1)) != PAPI_OK) {
	// ALTER:    fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
	// ALTER:    exit(1);
	// ALTER: }
	

	
	long int sum = 0;
	MyStruct *current = &global_buffer[0];
	for (int i = 0; i < ARRAY_SIZE; i++){
		sum += current->pad[0];
		current = current->n;
	}

	
	
	unsigned long long cpu_checkpoint_finish = rdtscp();
	// ALTER: if ((ret = PAPI_read_counters(values, 1)) != PAPI_OK) {
	// ALTER:   fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
	// ALTER:   exit(1);
	// ALTER: }
	
	
	// long int sum  = 0;
	// for (int i =0; i < ARRAY_SIZE; i++)
	// 	sum += global_buffer[i].pad[0];
	//
	
	
	// for (int i = 0; i < 199999; i++)
	// 	sum += new_array[i] * new_array[i + 1];
	
	double average_cpu_clocks = (double)(cpu_checkpoint_finish - cpu_checkpoint_start) / ARRAY_SIZE;
	std::cout << "Avg CPU Clocks:	" << average_cpu_clocks << std::endl;
	//std::cout << "Cache miss:	" << values[0] << std::endl;
	std::cout << "Sum:	" << sum << std::endl;
	
	// ************************************************
	
	return 0;
}


int Server::start_server () {
	tcp_port	= TCP_PORT[0];
	ib_port		= IB_PORT[0];
	ServerContext ctx[CLIENTS_CNT];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	char temp_char;	
	
	TEST_NZ(initialize_data_structures());	

	std::cout << "[Info] Server is waiting for " << CLIENTS_CNT
		<< " client(s) on tcp port: " << tcp_port << ", ib port: " << ib_port << std::endl;
	
	// Open Socket
	server_sockfd = open_socket();
	
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
		memcpy(&(ctx[i].send_message_msg), ctx[i].local_mr, sizeof(struct ibv_mr));
	}
	
	for (int i = 0; i < CLIENTS_CNT; i++){
		// send memory locations using SEND 
		TEST_NZ (RDMACommon::post_SEND (ctx[i].qp, ctx[i].send_message_mr, (uintptr_t)&ctx[i].send_message_msg, sizeof(struct ibv_mr), true));
		TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));
		DEBUG_COUT("[Sent] Memory region address  to client " << i);
	}
	
	
	TEST_NZ(start_benchmark());
	// Server waits for the client to muck with its memory
	
	for (int i = 0; i < CLIENTS_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
		DEBUG_COUT("[Conn] Client " << i << " notified it's finished");
		TEST_NZ (ctx[i].destroy_context());
		std::cout << "[Info] Destroying client " << i << " resources" << std::endl;
	}
	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;	
	return 0;
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		Server::usage(argv[0]);
		return 1;
	}
	Server server;
	server.start_server();
	return 0;
}