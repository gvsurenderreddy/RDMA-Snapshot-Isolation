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

int TimestampServer::initialize_data_structures() {
	read_timestamp.value = 0ULL;
	commit_timestamp.value = 1ULL;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Read and Commit timestamps set to " << read_timestamp.value << " and " << commit_timestamp.value << " respectively");
	return 0;
}

int TimestampServer::create_context(struct WorkerContext *ctx) {
	TEST_NZ(RDMACommon::build_connection(config::TIMESTAMP_SERVER_IB_PORT, &ctx->ib_ctx, &ctx->port_attr, &ctx->pd, &ctx->send_cq,&ctx->recv_cq, &ctx->send_comp_channel,&ctx->recv_comp_channel, 10));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx->ib_ctx, ctx->pd, ctx->send_cq, ctx->recv_cq, &(ctx->qp)));
	return 0;
}

int TimestampServer::register_memory(struct WorkerContext*ctx) {
	int readFlag	= IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ;
	int atomicFlag	= readFlag | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;

	TEST_Z(ctx->send_mr						= ibv_reg_mr(ctx->pd, &ctx->send_msg, sizeof(struct message::TimestampServerMemoryKeys), IBV_ACCESS_LOCAL_WRITE));
	TEST_Z(ctx->mr_transaction_result		= ibv_reg_mr(ctx->pd, &ctx->transaction_result, sizeof(struct message::TransactionResult), IBV_ACCESS_LOCAL_WRITE));
	TEST_Z(ctx->mr_transaction_result_ack	= ibv_reg_mr(ctx->pd, &ctx->transaction_result_ack, sizeof(struct message::TransactionResult), IBV_ACCESS_LOCAL_WRITE));
	TEST_Z(ctx->mr_commit_timestamp			= ibv_reg_mr(ctx->pd, &commit_timestamp, sizeof(Timestamp), atomicFlag));
	TEST_Z(ctx->mr_read_timestamp			= ibv_reg_mr(ctx->pd, &read_timestamp, sizeof(Timestamp), readFlag));
	return 0;
}

int TimestampServer::cleanUpFinishedTransactions() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Cleanup thread started.");

	while (true) {
		usleep(config::TIMESTAMP_CLEANUP_SLEEP_MILLISEC);

		std::unique_lock<std::mutex> locker(mu);
		cond.wait(locker, [this](){return finishedTrxs.size() > 0;});
		while (finishedTrxs.top().first.value == read_timestamp.value + 1) {
			avg_queue_size = (avg_queue_size * sample_size + (double)finishedTrxs.size()) / ((double)sample_size + 1);
			sample_size++;

			finishedTrxs.pop();
			read_timestamp.value = read_timestamp.value + 1;
			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Increment RTS to " << read_timestamp.value);
		}
		if (read_timestamp.value == config::CLIENTS_CNT * config::TRANSACTION_CNT)
			break;
		locker.unlock();
		cond.notify_all();
	}
	return 0;
}

int TimestampServer::appendTidToUnresolved(Timestamp &ts, Result &result) {
	std::unique_lock<std::mutex> locker(mu);
	cond.wait(locker, [this](){return finishedTrxs.size() < config::TIMESTAMP_SERVER_QUEUE_SIZE;});
	finishedTrxs.push(std::make_pair(ts,result));
	locker.unlock();
	cond.notify_all();
	return 0;
}

void TimestampServer::handle_client(struct WorkerContext*ctx) {
	struct message::TransactionResult trx_result;
	// create all resources
	TEST_NZ (create_context(ctx));
	
	TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->mr_transaction_result, (uintptr_t)&ctx->transaction_result, sizeof(struct message::TransactionResult)));
	
	// connect the QP to client
	TEST_NZ (RDMACommon::connect_qp (&(ctx->qp), config::TIMESTAMP_SERVER_IB_PORT, ctx->port_attr.lid, ctx->sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] TS-Server QPed to (" << get_full_desc(ctx) << ")");
	
	// send memory locations using SEND
	memcpy(&(ctx->send_msg.mr_read_timestamp),	ctx->mr_read_timestamp,	sizeof(struct ibv_mr));
	memcpy(&(ctx->send_msg.mr_commit_timestamp),ctx->mr_commit_timestamp,	sizeof(struct ibv_mr));
	TEST_NZ (RDMACommon::post_SEND (ctx->qp, ctx->send_mr, (uintptr_t)&ctx->send_msg, sizeof(struct message::TimestampServerMemoryKeys), true));
	TEST_NZ (RDMACommon::poll_completion(ctx->send_cq));
	DEBUG_COUT(CLASS_NAME, __func__, "[Sent] buffer info to client " << get_full_desc(ctx));
	

	/**********************************************************
	 * Waiting For Clients to submit their transaction results
	 **********************************************************/
	for (ctx->trx_num = 0; ctx->trx_num < config::TRANSACTION_CNT; ctx->trx_num++) {
		TEST_NZ (RDMACommon::poll_completion(ctx->recv_cq));		// completion for post_RECEIVE
		memcpy(&trx_result, &ctx->transaction_result, sizeof(struct message::TransactionResult));
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->mr_transaction_result, (uintptr_t)&ctx->transaction_result, sizeof(struct message::TransactionResult)));
		TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->mr_transaction_result_ack, (uintptr_t)&ctx->transaction_result_ack, sizeof(struct message::TransactionResult), true));
		TEST_NZ (RDMACommon::poll_completion(ctx->send_cq));		// completion for post_RECEIVE

		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] Transaction Result from (" << get_full_desc(ctx) << "): TID: " << ctx->transaction_result.ts.value << ", result: " << ctx->transaction_result.result);
		appendTidToUnresolved(trx_result.ts, trx_result.result);
	}

	// once it's finished with the client, it syncs with client to ensure that client is over
	char temp_char;
	TEST_NZ (sock_sync_data (ctx->sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	TEST_NZ (destroy_client_context(ctx));
}

std::string TimestampServer::get_full_desc(struct WorkerContext *ctx) {
	std::stringstream sstm;
	sstm << "ip: " << ctx->client_ip << ", port: " << ctx->client_port  << ", sock: " << ctx->sockfd;
	return sstm.str();
}


int TimestampServer::destroy_client_context (struct WorkerContext *ctx) {
	if (ctx->qp)
		TEST_NZ(ibv_destroy_qp (ctx->qp));
	
	if (ctx->send_mr)
		TEST_NZ (ibv_dereg_mr (ctx->send_mr));

	if (ctx->mr_transaction_result)
		TEST_NZ (ibv_dereg_mr (ctx->mr_transaction_result));

	if (ctx->mr_transaction_result_ack)
		TEST_NZ (ibv_dereg_mr (ctx->mr_transaction_result_ack));

	if (ctx->mr_read_timestamp)
		TEST_NZ (ibv_dereg_mr (ctx->mr_read_timestamp));
	
	if (ctx->mr_commit_timestamp)
		TEST_NZ (ibv_dereg_mr (ctx->mr_commit_timestamp));
	
	if (ctx->send_cq)
		TEST_NZ (ibv_destroy_cq (ctx->send_cq));

	if (ctx->recv_cq)
			TEST_NZ (ibv_destroy_cq (ctx->recv_cq));
	
	if (ctx->pd)
		TEST_NZ (ibv_dealloc_pd (ctx->pd));
	
	if (ctx->ib_ctx)
		TEST_NZ (ibv_close_device (ctx->ib_ctx));

	if (ctx->sockfd >= 0){
		TEST_NZ (close (ctx->sockfd));
	}
	return 0;
}

int TimestampServer::destroy_resources () {
	close(TimestampServer::server_sockfd);	// close the socket

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
	//pthread_t master_threads[CLIENTS_CNT];
	std::vector<std::thread> threads;
	std::thread cleanerThread;
	struct WorkerContext ctx[config::CLIENTS_CNT];
	
	std::thread cleanupThread(&TimestampServer::cleanUpFinishedTransactions, this);

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
	serv_addr.sin_port = htons(config::TIMESTAMP_SERVER_PORT);
	TEST_NZ(bind(TimestampServer::server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	
	// listen				  
	TEST_NZ(listen (TimestampServer::server_sockfd, config::CLIENTS_CNT));
	
	// accept connections from clients
	std::cout << "[Info] Waiting for " << config::CLIENTS_CNT << " client(s) on port " << config::TIMESTAMP_SERVER_PORT << std::endl;
	for (int c = 0; c < config::CLIENTS_CNT; c++){
		ctx[c].sockfd = accept (TimestampServer::server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[c].sockfd < 0) {
			std::cerr << "ERROR on accept() client #" << c << std::endl;
			return -1;
		}
		ctx[c].client_ip 	= "";
		ctx[c].client_ip	+= std::string(inet_ntoa (returned_addr.sin_addr));
		ctx[c].client_port	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client (" << get_full_desc(ctx) <<  ")" << std::endl;
		// connecting the QPs to resource-managers	
		//pthread_create(&master_threads[c], NULL, TimestampServer::handle_client, &ctx[c]);
		threads.push_back(std::thread(&TimestampServer::handle_client, this, &ctx[c]));
	}
	std::cout << "[Info] Established connection to all " << config::CLIENTS_CNT << " client(s)." << std::endl;
	
	//wait for handlers to finish
	for (auto& th : threads)
		th.join();
	
	cleanupThread.join();

	std::cout << "[Info] Server is done, now destroying resources!" << std::endl;
	TEST_NZ(destroy_resources());

	std::cout << "[Stat] Average queue size: " << avg_queue_size << std::endl;
	std::cout << "[Stat] Sample size: " << sample_size << std::endl;

	return 0;
}
