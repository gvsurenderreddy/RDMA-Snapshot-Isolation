/*
 *	RDMAServer.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#include "RDMAServer.hpp"

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

#define CLASS_NAME	"RDMAServer"

RDMAServer::~RDMAServer () {
	delete[](global_items_region);
	delete[](global_orders_region);
	delete[](global_order_line_region);
	delete[](global_cc_xacts_region);
	delete[](global_lock_items_region);

	close(server_sockfd_);
}

int RDMAServer::initialize_context(RDMAServerContext &ctx) {
	ctx.ib_port				= ib_port_;
	ctx.items_region		= global_items_region;
	ctx.orders_region		= global_orders_region;
	ctx.order_line_region	= global_order_line_region;
	ctx.cc_xacts_region		= global_cc_xacts_region;
	ctx.lock_items_region	= global_lock_items_region;
	return 0;
}

int RDMAServer::start_server () {
	RDMAServerContext ctx[clients_cnt_];

	std::cout << "[Info] Server " << server_num_ << " is waiting for " << clients_cnt_
			<< " client(s) on tcp port: " << tcp_port_ << ", ib port: " << ib_port_ << std::endl;

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	// Open Socket
	server_sockfd_ = socket (AF_INET, SOCK_STREAM, 0);
	if (server_sockfd_ < 0) {
		std::cerr << "Error opening socket" << std::endl;
		return -1;
	}

	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(tcp_port_);
	TEST_NZ(bind(server_sockfd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));


	// listen				  
	TEST_NZ(listen (server_sockfd_, clients_cnt_));

	// accept connections
	for (size_t i = 0; i < clients_cnt_; i++){
		initialize_context(ctx[i]);
		ctx[i].sockfd  = accept (server_sockfd_, (struct sockaddr *) &cli_addr, &clilen);
		if (ctx[i].sockfd < 0){ 
			std::cerr << "ERROR on accept" << std::endl;
			return -1;
		}
		std::cout << "[Conn] Received client #" << i << " on socket " << ctx[i].sockfd << std::endl;

		// create all resources
		TEST_NZ (ctx[i].create_context());
		DEBUG_COUT(CLASS_NAME, __func__, "[Info] Context for client " << i << " created");

		// connect the QPs
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));	
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to client " << i);

		// prepare server buffer with read message
		memcpy(&(ctx[i].send_msg.mr_items),		ctx[i].mr_items,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_orders),	ctx[i].mr_orders,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_order_line),ctx[i].mr_order_line,	sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_cc_xacts),	ctx[i].mr_cc_xacts,		sizeof(struct ibv_mr));
		memcpy(&(ctx[i].send_msg.mr_lock_items),ctx[i].mr_lock_items,	sizeof(struct ibv_mr));
	}

	for (size_t i = 0; i < clients_cnt_; i++){
		// send memory locations using SEND 
		TEST_NZ (RDMACommon::post_SEND (ctx[i].qp, ctx[i].send_mr, (uintptr_t)&ctx[i].send_msg, sizeof(struct message::DataServerMemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(ctx[i].send_cq));
		DEBUG_COUT(CLASS_NAME, __func__, "[Sent] buffer info to client " << i);
	}

	/*
		Server waits for the client to muck with its memory
	 */


	char temp_char;
	for (size_t i = 0; i < clients_cnt_; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Client " << i << " notified it's finished");
		TEST_NZ (ctx[i].destroy_context());
		std::cout << "[Info] Destroying client " << i << " resources" << std::endl;
	}
	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;
	return 0;
}

RDMAServer::RDMAServer(uint32_t server_num, uint32_t clients_cnt)
: clients_cnt_(clients_cnt),
  server_num_(server_num)
{
	server_sockfd_ = -1;
	tcp_port_	= config::TCP_PORT[server_num];
	ib_port_	= config::IB_PORT[server_num];

	global_items_region			= new ItemVersion[config::ITEM_CNT * config::MAX_ITEM_VERSIONS];

	//OrdersVersion*		RDMAServer::orders_region		= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
	global_orders_region		= new OrdersVersion[config::MAX_BUFFER_SIZE];	// TODO

	//OrderLineVersion*	RDMAServer::order_line_region	= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
	global_order_line_region	= new OrderLineVersion[config::MAX_BUFFER_SIZE];			// TODO

	//CCXactsVersion*		RDMAServer::cc_xacts_region		= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];
	global_cc_xacts_region		= new CCXactsVersion[config::MAX_BUFFER_SIZE];	// TODO

	global_lock_items_region	= new uint64_t[config::ITEM_CNT];

	TEST_NZ(load_tables_from_files(global_items_region));
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Tables loaded successfully");

	uint32_t stat, version;
	for (int i = 0; i < config::ITEM_CNT; i++) {
		stat = (uint32_t)0;	// 0 for free, 1 for locked
		version = (uint32_t)0;	// the first version of each item is 0
		global_lock_items_region[i] = Lock::set_lock(stat, version);
	}
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] All locks set free");
}
