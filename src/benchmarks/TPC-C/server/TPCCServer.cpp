/*
 * TPCCServer.cpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#include "TPCCServer.hpp"
#include "../random/randomgenerator.hpp"
#include "../../../rdma-region/MemoryHandler.hpp"
#include "../tables/TPCCUtil.hpp"
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <iostream>
#include <netdb.h>	// for socket-related functions and variables
//#include <sys/types.h>	// for listen
#include <sys/socket.h>

using namespace config::tpcc_settings;

#define CLASS_NAME	"TPCCServer"


TPCCServer::TPCCServer(uint32_t serverNum, unsigned instanceNum, uint32_t clientsCnt)
: serverNum_(serverNum),
  instanceNum_(instanceNum),
  clientsCnt_(clientsCnt),
  tcp_port_(config::TCP_PORT[serverNum]),
  ib_port_(config::IB_PORT[serverNum]){


	TPCC::RealRandomGenerator random;
	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random);
	random.setC(cLoad);


	context_ = new RDMAContext(ib_port_);


	db = new TPCCDB(
			WAREHOUSE_PER_SERVER,
			DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			CUSTOMER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			ORDER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			tpcc_settings::ORDER_MAX_OL_CNT * ORDER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			NEWORDER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			STOCK_PER_WAREHOUSE * WAREHOUSE_PER_SERVER,
			ITEMS_CNT,
			VERSION_NUM,
			random,
			*context_);
	db->populate();

	// Put the memory keys into the message that is to be sent to clients
	memoryKeysMessage_ = new RDMARegion<ServerMemoryKeys>(1, *context_, IBV_ACCESS_LOCAL_WRITE);
	db->getMemoryKeys(memoryKeysMessage_->getRegion());
	memoryKeysMessage_->getRegion()->serverInstanceNum = instanceNum_;

	std::cout << "[Info] Server " << serverNum_ << " is waiting for " << clientsCnt_
			<< " client(s) on tcp port: " << tcp_port_ << ", ib port: " << (int)ib_port_ << std::endl;

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);

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
	TEST_NZ(listen (server_sockfd_, clientsCnt_));

	// accept connections
	for (size_t i = 0; i < clientsCnt_; i++){
		int sockfd;
		sockfd = accept (server_sockfd_, (struct sockaddr *) &cli_addr, &clilen);
		if (sockfd < 0){
			std::cerr << "ERROR on accept" << std::endl;
			exit(-1);
		}
		std::cout << "[Conn] Received client #" << i << " on socket " << sockfd << std::endl;
		clientCtxs.push_back(new ClientContext(sockfd, *context_));

		// connect the QPs
		clientCtxs[i]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Established QP to client " << i);
	}

	for (size_t i = 0; i < clientsCnt_; i++){
		// send memory locations using SEND
		TEST_NZ (RDMACommon::post_SEND (clientCtxs[i]->getQP(), memoryKeysMessage_->getRDMAHandler(), (uintptr_t)memoryKeysMessage_->getRegion(), sizeof(struct ServerMemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Sent] buffer info to client " << i);
	}

	/*
		Server waits for the client to muck with its memory
	 */

	for (size_t i = 0; i < clientsCnt_; i++) {
		TEST_NZ (utils::sock_sync (clientCtxs[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Client " << i << " notified it's finished");
		delete clientCtxs[i];
	}

	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;
}

TPCCServer::~TPCCServer() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete memoryKeysMessage_;
	delete db;
	delete context_;
}
