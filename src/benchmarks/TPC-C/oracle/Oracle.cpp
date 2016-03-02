/*
 *	Oracle.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../../../util/utils.hpp"
#include "../../../util/RDMACommon.hpp"
#include "WorkerContext.hpp"
#include <stdio.h>
#include <cstring>	// for memcpy and memset
#include <string>
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
#include <vector>
#include <infiniband/verbs.h>	// for ibv_qp
#include "Oracle.hpp"


#define CLASS_NAME	"TS"

TPCC::Oracle::Oracle(size_t clients_cnt)
: clients_cnt_ (clients_cnt),
  server_sockfd_ (-1),
  tcp_port_(config::TIMESTAMP_SERVER_PORT),
  ib_port_(config::TIMESTAMP_SERVER_IB_PORT){

	std::vector<WorkerContext*> workerCtxs;


	context_ = new RDMAContext(ib_port_);
	lastCommittedVector_ = new RDMARegion<primitive::timestamp_t>(clients_cnt_, *context_, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC);
	for (size_t i = 0; i < clients_cnt_; i++)
		lastCommittedVector_->getRegion()[i] = (primitive::timestamp_t)0;

	struct sockaddr_in serv_addr, returned_addr;
	socklen_t len = sizeof(returned_addr);

	// Open Socket
	server_sockfd_ = socket (AF_INET, SOCK_STREAM, 0);
	if (server_sockfd_ < 0) {
		std::cerr << "Error opening socket" << std::endl;
		exit(-1);
	}

	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(tcp_port_);
	TEST_NZ(bind(server_sockfd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));

	// listen				  
	TEST_NZ(listen (server_sockfd_, (int)clients_cnt_));

	// accept connections from clients
	std::cout << "[Info] Waiting for " << clients_cnt_ << " client(s) on port " << tcp_port_ << std::endl;
	for (primitive::client_id_t c = 0; c < clients_cnt_; c++){
		int sockfd = accept (server_sockfd_, (struct sockaddr *) &returned_addr, &len);
		if (sockfd < 0) {
			std::cerr << "ERROR on accept() client #" << c << std::endl;
			exit(-1);
		}
		workerCtxs.push_back(new WorkerContext(sockfd, *context_));

		// connect the QPs
		workerCtxs[c]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Established QP to client " << c);

		workerCtxs[c]->clientIp_ = std::string(inet_ntoa (returned_addr.sin_addr));
		workerCtxs[c]->clientPort_ = (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client (" << *workerCtxs[c] <<  ")" << std::endl;

		// prepare the message before sending to client
		workerCtxs[c]->getMemoryKeysMessage()->getRegion()->client_cnt = clients_cnt;
		workerCtxs[c]->getMemoryKeysMessage()->getRegion()->client_id = c;
		lastCommittedVector_->getMemoryHandler(workerCtxs[c]->getMemoryKeysMessage()->getRegion()->lastCommittedVector);
	}

	for (size_t c = 0; c < clients_cnt_; c++){
		// send memory locations using SEND
		TEST_NZ (RDMACommon::post_SEND (workerCtxs[c]->getQP(), workerCtxs[c]->getMemoryKeysMessage()->getRDMAHandler(), (uintptr_t)workerCtxs[c]->getMemoryKeysMessage()->getRegion(), sizeof(struct OracleMemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Sent] buffer info sent to client " << *workerCtxs[c]);
	}

	// Destroy clients' resources
	for (size_t c = 0; c < clients_cnt_; c++) {
		// once it's finished with the client, it syncs with client to ensure that client is over
		TEST_NZ (utils::sock_sync (workerCtxs[c]->getSockFd()));	// just send a dummy char back and forth
		delete workerCtxs[c];
	}

	std::cout << "[Info] Server is done." << std::endl;
}

TPCC::Oracle::~Oracle(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete lastCommittedVector_;
	delete context_;
	close(server_sockfd_);
}
