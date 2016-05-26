/*
 *	Oracle.cpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "Oracle.hpp"
#include "../../util/utils.hpp"
#include "../../rdma-region/RDMACommon.hpp"
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
#include <fstream>      // std::ofstream


#define CLASS_NAME	"Oracle"

Oracle::Oracle(size_t clientsCnt, size_t instancesCnt)
: clientsCnt_ (clientsCnt),
  instancesCnt_(instancesCnt),
  server_sockfd_ (-1),
  tcp_port_(config::TIMESTAMP_SERVER_PORT),
  ib_port_(config::TIMESTAMP_SERVER_IB_PORT){

	if (config::Output::FILE == DEBUG_OUTPUT) {
		std::string filename = std::string(config::LOG_FOLDER) + "/oracle.log";
		os_ = new std::ofstream (filename, std::ofstream::out);
	}
	else os_ = &std::cout;

	std::vector<WorkerContext*> workerCtxs;
	context_ = new RDMAContext(*os_, ib_port_);
	lastCommittedVector_ = new RDMARegion<primitive::timestamp_t>(clientsCnt_, *context_, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC);
	for (size_t i = 0; i < clientsCnt_; i++)
		lastCommittedVector_->getRegion()[i] = (primitive::timestamp_t)0;

	struct sockaddr_in returned_addr;
	socklen_t len = sizeof(returned_addr);

	int numberOfConnections = (int)(clientsCnt_ + instancesCnt_);
	server_sockfd_ = utils::server_socket_setup(tcp_port_, numberOfConnections);
	PRINT_COUT(CLASS_NAME, __func__, "[Info] Waiting for " << instancesCnt_ << " instances(s) and " << clientsCnt_ << " clients on port " << tcp_port_);

	primitive::client_id_t clientID = 0;
	for (size_t c = 0; c < instancesCnt_ + clientsCnt_; c++){
		int sockfd = accept (server_sockfd_, (struct sockaddr *) &returned_addr, &len);
		if (sockfd < 0) {
			PRINT_CERR(CLASS_NAME, __func__, "ERROR on accept() client #" << c );
			exit(-1);
		}

		char type;
		TEST_NZ(utils::sock_read(sockfd, &type, sizeof(char)));

		workerCtxs.push_back(new WorkerContext(*os_, sockfd, *context_));

		// connect the QPs
		workerCtxs[c]->activateQueuePair(*context_);
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Established QP to client " << c);

		workerCtxs[c]->clientIp_ = std::string(inet_ntoa (returned_addr.sin_addr));
		workerCtxs[c]->clientPort_ = (int) ntohs(returned_addr.sin_port);
		PRINT_COUT(CLASS_NAME, __func__, "[Conn] Received client (" << *workerCtxs[c] <<  ")");

		// prepare the message before sending to client
		workerCtxs[c]->getMemoryKeysMessage()->getRegion()->client_cnt = clientsCnt;
		if (type == 'c') {
			workerCtxs[c]->getMemoryKeysMessage()->getRegion()->client_id = clientID;
			DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] received client " << clientID);
			clientID++;
		}
		else if (type == 'i')
			DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] received instance");

		lastCommittedVector_->getMemoryHandler(workerCtxs[c]->getMemoryKeysMessage()->getRegion()->lastCommittedVector);
	}

	for (size_t c = 0; c < instancesCnt_ + clientsCnt_; c++){
		// send memory locations using SEND
		TEST_NZ (RDMACommon::post_SEND (workerCtxs[c]->getQP(), workerCtxs[c]->getMemoryKeysMessage()->getRDMAHandler(), (uintptr_t)workerCtxs[c]->getMemoryKeysMessage()->getRegion(), sizeof(struct OracleMemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Sent] buffer info sent to client " << *workerCtxs[c]);
	}

	// Destroy clients' resources
	for (size_t c = 0; c < instancesCnt_ + clientsCnt_; c++) {
		// once it's finished with the client, it syncs with client to ensure that client is over
		TEST_NZ (utils::sock_sync (workerCtxs[c]->getSockFd()));	// just send a dummy char back and forth
		delete workerCtxs[c];
	}

	PRINT_COUT(CLASS_NAME, __func__, "[Info] Oracle is done.");
}

Oracle::~Oracle(){
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete lastCommittedVector_;
	delete context_;
	close(server_sockfd_);

	// if os_ == &std::cout, deleting os_ will result in core dumped
	if (os_ != &std::cout)
		delete os_;
}
