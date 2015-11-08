/*
 *	RDMAServer.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "RDMAServer.hpp"
#include "../../config.hpp"
#include "../util/utils.hpp"
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

RDMAServer::~RDMAServer () {
	delete[](global_items_region);
	delete[](global_orders_region);
	delete[](global_order_line_region);
	delete[](global_cc_xacts_region);
	delete[](global_timestamp_region);
	delete[](global_lock_items_region);
	
	close(server_sockfd);
}

int RDMAServer::initialize_data_structures(){
	global_items_region			= new ItemVersion[ITEM_CNT * MAX_ITEM_VERSIONS];
	
	//OrdersVersion*		RDMAServer::orders_region		= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
	global_orders_region		= new OrdersVersion[MAX_BUFFER_SIZE];	// TODO

	//OrderLineVersion*	RDMAServer::order_line_region	= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
	global_order_line_region	= new OrderLineVersion[MAX_BUFFER_SIZE];			// TODO
	
	//CCXactsVersion*		RDMAServer::cc_xacts_region		= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];
	global_cc_xacts_region		= new CCXactsVersion[MAX_BUFFER_SIZE];	// TODO
	
	global_timestamp_region		= new Timestamp[1];
	global_lock_items_region	= new uint64_t[ITEM_CNT];
	
	
	
	RDMAServer::global_timestamp_region[0].value = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("[Info] Timestamp set to " << RDMAServer::global_timestamp_region[0].value);

	TEST_NZ(load_tables_from_files(global_items_region));
	DEBUG_COUT("[Info] Tables loaded successfully");
	
	int i;
	uint32_t stat;
	uint32_t version;
	for (i=0; i < ITEM_CNT; i++) {
		stat = (uint32_t)0;	// 0 for free, 1 for locked
		version = (uint32_t)0;	// the first version of each item is 0
		global_lock_items_region[i] = Lock::set_lock(stat, version);
	}
	DEBUG_COUT("[Info] All locks set free");
		
	return 0;
}

int RDMAServer::initialize_context(RDMAServerContext &ctx) {
	ctx.ib_port				= ib_port;
	
	ctx.items_region		= global_items_region;
	ctx.orders_region		= global_orders_region;
	ctx.order_line_region	= global_order_line_region;
	ctx.cc_xacts_region		= global_cc_xacts_region;
	ctx.timestamp_region	= global_timestamp_region;
	ctx.lock_items_region	= global_lock_items_region;
	return 0;
}


void RDMAServer::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " <i = server_num>" << std::endl;
	std::cout << "starts a server and waits for connection on port Config.TCP_PORT[i]" << std::endl;
	std::cout << "(valid range of i: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
}

int RDMAServer::start_server (int server_num) {	
	tcp_port	= TCP_PORT[server_num];
	ib_port		= IB_PORT[server_num];
	RDMAServerContext ctx[CLIENTS_CNT];
	
	char temp_char;
	
	
	TEST_NZ(initialize_data_structures());

	std::cout << "[Info] Server " << server_num << " is waiting for " << CLIENTS_CNT
		<< " client(s) on tcp port: " << tcp_port << ", ib port: " << ib_port << std::endl;
	
	server_sockfd = -1;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	pthread_t master_threads[CLIENTS_CNT];	
	
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
		// pthread_create(&master_threads[i], NULL, RDMAServer::handle_client, &client_socks[i]);
	
		// create all resources
		TEST_NZ (ctx[i].create_context());
		DEBUG_COUT("[Info] Context for client " << i << " created");
	
		// connect the QPs
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));	
		DEBUG_COUT("[Conn] QPed to client " << i);
	
		// prepare server buffer with read message
		memcpy(&(ctx[i].send_msg.mr_items),		ctx[i].mr_items,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_orders),	ctx[i].mr_orders,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_order_line),ctx[i].mr_order_line,	sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_cc_xacts),	ctx[i].mr_cc_xacts,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_timestamp),	ctx[i].mr_timestamp,	sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_lock_items),ctx[i].mr_lock_items,	sizeof(struct ibv_mr));
	}
	
	for (int i = 0; i < CLIENTS_CNT; i++){
	
		// send memory locations using SEND 
		TEST_NZ (RDMACommon::post_SEND (ctx[i].qp, ctx[i].send_mr, (uintptr_t)&ctx[i].send_msg, sizeof(struct MemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));
		DEBUG_COUT("[Sent] buffer info to client " << i);
	}

	/*
		Server waits for the client to muck with its memory
	*/

	for (int i = 0; i < CLIENTS_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT("[Conn] Client " << i << " notified it's finished");
		TEST_NZ (ctx[i].destroy_context());
		std::cout << "[Info] Destroying client " << i << " resources" << std::endl;
	}
	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;	
}

int main (int argc, char *argv[]) {
	if (argc != 2) {
		RDMAServer::usage(argv[0]);
		return 1;
	}
	RDMAServer server;
	server.start_server(atoi(argv[1]));
	return 0;
}