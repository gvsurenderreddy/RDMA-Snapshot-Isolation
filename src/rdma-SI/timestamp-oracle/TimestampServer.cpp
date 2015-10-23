/*
 *	timestamp-server.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "TimestampServer.hpp"

#include "../../util/utils.hpp"
#include "../../util/RDMACommon.hpp"
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
#include <thread>         // std::thread


#define CLASS_NAME	"TS"


int TimestampServer::create_context(struct WorkerContext &ctx) {
	TEST_NZ(RDMACommon::build_connection(config::TIMESTAMP_SERVER_IB_PORT, &ctx.ib_ctx, &ctx.port_attr, &ctx.pd, &ctx.send_cq,&ctx.recv_cq, &ctx.send_comp_channel,&ctx.recv_comp_channel, 10));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx.ib_ctx, ctx.pd, ctx.send_cq, ctx.recv_cq, &(ctx.qp)));
	return 0;
}

int TimestampServer::register_memory(struct WorkerContext &ctx) {
	int readFlag	= IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ;
	int atomicFlag	= readFlag | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;

	TEST_Z(ctx.mr_send				= ibv_reg_mr(ctx.pd, &ctx.send_msg, 		sizeof(struct message::TimestampServerMemoryKeys), IBV_ACCESS_LOCAL_WRITE));
	TEST_Z(ctx.mr_finished_trxs_hash= ibv_reg_mr(ctx.pd, finished_trxs_hash_,	sizeof(uint8_t) * config::TIMESTAMP_SERVER_QUEUE_SIZE, atomicFlag));
	TEST_Z(ctx.mr_commit_timestamp	= ibv_reg_mr(ctx.pd, &commit_timestamp_,	sizeof(Timestamp), atomicFlag));
	TEST_Z(ctx.mr_read_timestamp	= ibv_reg_mr(ctx.pd, &read_timestamp_,		sizeof(Timestamp), readFlag));

	return 0;
}

int TimestampServer::cleanUpFinishedTransactions() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Cleanup thread started.");
	while (read_timestamp_.value < clients_cnt_ * config::TRANSACTION_CNT) {
		usleep(config::TIMESTAMP_CLEANUP_SLEEP_MICROSEC);

		// check if the next timestamp is ready. If so, advance RTS. If not, wait.
		size_t next_index;
		while (true) {
			next_index = (read_timestamp_.value + 1) % config::TIMESTAMP_SERVER_QUEUE_SIZE;
			if (finished_trxs_hash_[next_index] == 1) {
				finished_trxs_hash_[next_index] = 0;
				read_timestamp_.value += 1;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Increment RTS to " << read_timestamp.value);
			}
			else break;
		}
	}
	return 0;
}

void TimestampServer::handle_client(struct WorkerContext &ctx) {
	// create all resources
	TEST_NZ (create_context(ctx));

	// connect the QP to client
	TEST_NZ (RDMACommon::connect_qp (&(ctx.qp), config::TIMESTAMP_SERVER_IB_PORT, ctx.port_attr.lid, ctx.sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] TS-Server QPed to (" << get_full_desc(ctx) << ")");
	
	// send memory locations using SEND
	memcpy(&(ctx.send_msg.mr_finished_trxs_hash),	ctx.mr_finished_trxs_hash,	sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg.mr_read_timestamp),		ctx.mr_read_timestamp,	sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg.mr_commit_timestamp),		ctx.mr_commit_timestamp,	sizeof(struct ibv_mr));

	TEST_NZ (RDMACommon::post_SEND (ctx.qp, ctx.mr_send, (uintptr_t)&ctx.send_msg, sizeof(struct message::TimestampServerMemoryKeys), true));
	TEST_NZ (RDMACommon::poll_completion(ctx.send_cq));
	DEBUG_COUT(CLASS_NAME, __func__, "[Sent] buffer info sent to client " << get_full_desc(ctx));
}

std::string TimestampServer::get_full_desc(struct WorkerContext &ctx) {
	std::stringstream sstm;
	sstm << "IP: " << ctx.client_ip << ", port: " << ctx.client_port  << ", sock: " << ctx.sockfd;
	return sstm.str();
}


int TimestampServer::destroy_client_context (struct WorkerContext &ctx) {
	if (ctx.qp)
		TEST_NZ(ibv_destroy_qp (ctx.qp));
	
	if (ctx.mr_send)
		TEST_NZ (ibv_dereg_mr (ctx.mr_send));

	if (ctx.mr_finished_trxs_hash)
		TEST_NZ (ibv_dereg_mr (ctx.mr_finished_trxs_hash));

	if (ctx.mr_read_timestamp)
		TEST_NZ (ibv_dereg_mr (ctx.mr_read_timestamp));
	
	if (ctx.mr_commit_timestamp)
		TEST_NZ (ibv_dereg_mr (ctx.mr_commit_timestamp));
	
	if (ctx.send_cq)
		TEST_NZ (ibv_destroy_cq (ctx.send_cq));

	if (ctx.recv_cq)
		TEST_NZ (ibv_destroy_cq (ctx.recv_cq));
	
	if (ctx.pd)
		TEST_NZ (ibv_dealloc_pd (ctx.pd));
	
	if (ctx.ib_ctx)
		TEST_NZ (ibv_close_device (ctx.ib_ctx));

	if (ctx.sockfd >= 0){
		TEST_NZ (close (ctx.sockfd));
	}
	return 0;
}

int TimestampServer::destroy_resources () {
	close(server_sockfd_);	// close the socket

	return 0;
}

void TimestampServer::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " starts a timestamp server and waits for connection" << std::endl;
}

int TimestampServer::start_server () {
	struct sockaddr_in serv_addr, returned_addr;
	socklen_t len = sizeof(returned_addr);
	std::thread cleanerThread;
	struct WorkerContext ctx[clients_cnt_];
	
	std::thread cleanupThread(&TimestampServer::cleanUpFinishedTransactions, this);

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
	serv_addr.sin_port = htons(config::TIMESTAMP_SERVER_PORT);
	TEST_NZ(bind(server_sockfd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	
	// listen				  
	TEST_NZ(listen (server_sockfd_, clients_cnt_));
	
	// accept connections from clients
	std::cout << "[Info] Waiting for " << clients_cnt_ << " client(s) on port " << config::TIMESTAMP_SERVER_PORT << std::endl;
	for (size_t c = 0; c < clients_cnt_; c++){
		ctx[c].sockfd = accept (server_sockfd_, (struct sockaddr *) &returned_addr, &len);
		if (ctx[c].sockfd < 0) {
			std::cerr << "ERROR on accept() client #" << c << std::endl;
			return -1;
		}
		ctx[c].client_ip 	= "";
		ctx[c].client_ip	+= std::string(inet_ntoa (returned_addr.sin_addr));
		ctx[c].client_port	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client (" << get_full_desc(ctx[c]) <<  ")" << std::endl;
		// connecting the QPs to resource-managers	
		handle_client(ctx[c]);
	}
	std::cout << "[Info] Established connection to all " << clients_cnt_ << " client(s)." << std::endl;
	
	//wait for the cleanup thread to finish
	cleanupThread.join();


	// Destroy clients' resources
	char temp_char;
	for (size_t c = 0; c < clients_cnt_; c++) {
		// once it's finished with the client, it syncs with client to ensure that client is over
		TEST_NZ (sock_sync_data (ctx[c].sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
		TEST_NZ (destroy_client_context(ctx[c]));
	}

	std::cout << "[Info] Server is done, now destroying resources!" << std::endl;
	TEST_NZ(destroy_resources());
	return 0;
}

TimestampServer::TimestampServer(uint32_t clients_cnt)
: clients_cnt_ (clients_cnt),
  server_sockfd_ (-1) {
	read_timestamp_.value = 0ULL;
	commit_timestamp_.value = 1ULL;
	for (size_t i = 0; i < config::TIMESTAMP_SERVER_QUEUE_SIZE; i++)
		finished_trxs_hash_[i] = 0;

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Read and Commit timestamps set to " << read_timestamp.value << " and " << commit_timestamp.value << " respectively");
}
