/*
 *	timestamp-server.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "timestamp-server.hpp"
#include "../util/utils.hpp"
#include "../util/RDMACommon.hpp"
#include <stdio.h>
#include <string.h>
#include <sstream>	// for stringstream
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>


int			TimestampServer::server_sockfd;
Timestamp	TimestampServer::timestamp;
std::mutex 	TimestampServer::timestamp_mutex;


int TimestampServer::create_context(struct Context *ctx) {	
	TEST_NZ(RDMACommon::build_connection(TIMESTAMP_SERVER_IB_PORT, &ctx->ib_ctx, &ctx->port_attr, &ctx->pd, &ctx->cq, 10));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx->ib_ctx, ctx->pd, ctx->cq, &(ctx->qp)));
	return 0;
}

int TimestampServer::register_memory(struct Context *ctx) {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	TEST_Z(ctx->timestamp_req_mr	= ibv_reg_mr(ctx->pd, &ctx->timestamp_request, sizeof(int), mr_flags));
	TEST_Z(ctx->timestamp_res_mr	= ibv_reg_mr(ctx->pd, &ctx->timestamp_response, sizeof(struct TimestampResponse), mr_flags));
	return 0;
}

int TimestampServer::acquire_commit_timestamp(Timestamp *commit_timestamp) {
	timestamp_mutex.lock();
	timestamp.value++;
	memcpy(commit_timestamp, &(TimestampServer::timestamp), sizeof(Timestamp));
	timestamp_mutex.unlock();
}

void* TimestampServer::handle_client(void *param) {
	Context *ctx = (Context*)param;
	
	// create all resources
	TEST_NZ (create_context(ctx));
	
	TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->timestamp_req_mr, (uintptr_t)&ctx->timestamp_request, sizeof(struct TimestampRequest)));
	
	// connect the QP to client
	TEST_NZ (RDMACommon::connect_qp (&(ctx->qp), TRX_MANAGER_IB_PORT, ctx->port_attr.lid, ctx->client_sockfd));
	
	DEBUG_COUT("[Conn] TS-Server QPed to (" << get_full_desc(ctx) << ")");
	
	for (ctx->trx_num = 0; ctx->trx_num < TRANSACTION_CNT; ctx->trx_num++) {
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_RECEIVE
		DEBUG_COUT("[Recv] Timestamp request from (" << get_full_desc(ctx) << ")");
		acquire_commit_timestamp(&ctx->timestamp_response.commit_timestamp);
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->timestamp_req_mr, (uintptr_t)&ctx->timestamp_request, sizeof(struct TimestampRequest)));
		TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->timestamp_res_mr, (uintptr_t)&ctx->timestamp_response, sizeof(struct TimestampResponse), true));
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_SEND
		DEBUG_COUT("[Sent] Timestamp Response to (" << get_full_desc(ctx) << ")");
	}
		
	TEST_NZ (destroy_client_context(ctx));
	return NULL;
}

std::string TimestampServer::get_full_desc(struct Context *ctx) {
	std::stringstream sstm;
	sstm << "ip: " << ctx->client_ip << ", port: " << ctx->client_port  << ", sock: " << ctx->client_sockfd; 
	return sstm.str();
}


int TimestampServer::destroy_client_context (struct Context *ctx) {
	if (ctx->qp)
		TEST_NZ(ibv_destroy_qp (ctx->qp));
	
	if (ctx->timestamp_req_mr)
		TEST_NZ (ibv_dereg_mr (ctx->timestamp_req_mr));
	
	if (ctx->timestamp_res_mr)
		TEST_NZ (ibv_dereg_mr (ctx->timestamp_res_mr));
	
	if (ctx->cq)
		TEST_NZ (ibv_destroy_cq (ctx->cq));
	
	if (ctx->pd)
		TEST_NZ (ibv_dealloc_pd (ctx->pd));
	
	if (ctx->ib_ctx)
		TEST_NZ (ibv_close_device (ctx->ib_ctx));

	if (ctx->client_sockfd >= 0){
		TEST_NZ (close (ctx->client_sockfd));
	}
}


int TimestampServer::destroy_resources () {
	close(TimestampServer::server_sockfd);	// close the socket
	return 0;
}


int TimestampServer::initialize_data_structures(){
	
	TimestampServer::timestamp.value = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("[Info] Timestamp set to " << TimestampServer::timestamp.value);
	return 0;
}

void TimestampServer::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " starts a timestamp server and waits for connection" << std::endl;
}

int TimestampServer::start_server () {	
	TEST_NZ(initialize_data_structures());
	
	TimestampServer::server_sockfd = -1;
	struct sockaddr_in serv_addr, returned_addr;
	socklen_t len = sizeof(returned_addr);
	pthread_t master_threads[CLIENTS_CNT];
	struct Context ctx[CLIENTS_CNT];
	
	// Open Socket
	TimestampServer::server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (TimestampServer::server_sockfd < 0) {
		std::cerr << "Error opening socket" << std::endl;
		return -1;
	}
	
	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TIMESTAMP_SERVER_PORT);
	TEST_NZ(bind(TimestampServer::server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	
	// listen				  
	TEST_NZ(listen (TimestampServer::server_sockfd, CLIENTS_CNT));
	
	// accept connections from clients
	std::cout << std::endl << "[Info] Waiting for " << CLIENTS_CNT << " client(s) on port " << TIMESTAMP_SERVER_PORT << std::endl;	
	for (int c = 0; c < CLIENTS_CNT; c++){
		ctx[c].client_sockfd = accept (TimestampServer::server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[c].client_sockfd < 0) {
			std::cerr << "ERROR on accept() client #" << c << std::endl;
			return -1;
		}
		ctx[c].client_ip 	= "";
		ctx[c].client_ip	+= std::string(inet_ntoa (returned_addr.sin_addr));
		ctx[c].client_port	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client (" << get_full_desc(ctx) <<  ")" << std::endl;
		// connecting the QPs to resource-managers	
		pthread_create(&master_threads[c], NULL, TimestampServer::handle_client, &ctx[c]);
	}
	
	std::cout << "[Info] Established connection to all " << CLIENTS_CNT << " client(s)." << std::endl; 
	
	
	//wait for handlers to finish
	for (int i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	
	// close server socket
	std::cout << "[Info] Server is done, now destroying resources!" << std::endl;
	TEST_NZ(destroy_resources());
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		TimestampServer::usage(argv[0]);
		return 1;
	}
	TimestampServer server;
	server.start_server();
	return 0;
}