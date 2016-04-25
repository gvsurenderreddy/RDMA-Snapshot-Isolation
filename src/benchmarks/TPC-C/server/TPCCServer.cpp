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
#include <fstream>      // std::ofstream


using namespace config::tpcc_settings;

#define CLASS_NAME	"TPCCServer"


bool TPCC::TPCCServer::threadsActiveStateFlag[config::SERVER_THREADS_CNT];

TPCC::TPCCServer::TPCCServer(uint32_t serverNum, unsigned instanceNum, uint32_t clientsCnt)
: serverNum_(serverNum),
  instanceNum_(instanceNum),
  clientsCnt_(clientsCnt),
  tcp_port_(config::TCP_PORT[serverNum]),
  ib_port_(config::IB_PORT[serverNum]),
  liveClientCnt_(clientsCnt){

	if (config::Output::FILE == DEBUG_OUTPUT) {
		std::string filename = std::string(config::LOG_FOLDER) + "/server_" + std::to_string(serverNum_) + ".log";
		os_ = new std::ofstream (filename, std::ofstream::out);
	}
	else os_ = &std::cout;

	TPCC::RealRandomGenerator random;
	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random);
	random.setC(cLoad);

	context_ = new RDMAContext(*os_, ib_port_);

	PRINT_COUT(CLASS_NAME, __func__, "[Info] Server " << serverNum_ << " is populating its database. Please do not start the clients before this process is over .... ");

	size_t warehouseTableSize = WAREHOUSE_PER_SERVER;
	size_t districtTableSize = DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t customerTableSize = CUSTOMER_PER_DISTRICT * DISTRICT_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t orderTableSize = clientsCnt_ * ORDER_BUFFER_PER_CLIENT;
	size_t orderLineTableSize = clientsCnt_ * tpcc_settings::ORDER_MAX_OL_CNT * ORDER_BUFFER_PER_CLIENT;
	size_t newOrderTableSize = clientsCnt_ * ORDER_BUFFER_PER_CLIENT;
	size_t stockTableSize = STOCK_PER_WAREHOUSE * WAREHOUSE_PER_SERVER;
	size_t itemTableSize = ITEMS_CNT;
	size_t historyTableSize = clientsCnt_ * HISTORY_BUFFER_PER_CLIENT;
	size_t versionNum = VERSION_NUM;

	std::vector<uint16_t> warehouseIDs;
	for (size_t i = 0; i < config::tpcc_settings::WAREHOUSE_PER_SERVER; i++)
		warehouseIDs.push_back((uint16_t)(serverNum_ * config::tpcc_settings::WAREHOUSE_PER_SERVER + i));

	db = new TPCC::TPCCDB(*os_, warehouseIDs, warehouseTableSize, districtTableSize, customerTableSize, orderTableSize, orderLineTableSize, newOrderTableSize, stockTableSize, itemTableSize, historyTableSize, versionNum, random, *context_);

	PRINT_COUT(CLASS_NAME, __func__, "[Info] Server " << serverNum_ << "'s database is successfully loaded");


	// Put the memory keys into the message that is to be sent to clients
	memoryKeysMessage_ = new RDMARegion<ServerMemoryKeys>(1, *context_, IBV_ACCESS_LOCAL_WRITE);
	db->getMemoryKeys(memoryKeysMessage_->getRegion());
	memoryKeysMessage_->getRegion()->serverInstanceNum = instanceNum_;

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

	PRINT_COUT(CLASS_NAME, __func__, "[Info] Server " << serverNum_ << " is waiting for " << clientsCnt_ << " client(s) on tcp port: " << tcp_port_ << ", ib port: " << (int)ib_port_);

	// accept connections
	for (size_t c = 0; c < clientsCnt_; c++){
		int sockfd;
		sockfd = accept (server_sockfd_, (struct sockaddr *) &cli_addr, &clilen);
		if (sockfd < 0){
			std::cerr << "ERROR on accept" << std::endl;
			exit(-1);
		}
		PRINT_COUT(CLASS_NAME, __func__, "[Conn] Received client #" << c << " on socket " << sockfd);
		clientCtxs.push_back(new ClientContext(*os_, sockfd, *context_));

		// connect the QPs
		clientCtxs[c]->activateQueuePair(*context_);
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Established QP to client " << c);

		uint32_t qp_num = clientCtxs[c]->getQP()->qp_num;
		qpNum_to_clientIndex_map[qp_num] = (primitive::client_id_t)c;
	}

	for (size_t c = 0; c < clientsCnt_; c++){
		// post indexRequestMessage prior to establish the connection to the client
		TEST_NZ (RDMACommon::post_RECEIVE (
				clientCtxs[c]->getQP(),
				clientCtxs[c]->getIndexRequestMessage()->getRDMAHandler(),
				(uintptr_t)clientCtxs[c]->getIndexRequestMessage()->getRegion(),
				sizeof(IndexRequestMessage)));

		// send memory locations using SEND
		TEST_NZ (RDMACommon::post_SEND (clientCtxs[c]->getQP(), memoryKeysMessage_->getRDMAHandler(), (uintptr_t)memoryKeysMessage_->getRegion(), sizeof(struct ServerMemoryKeys), true));
		TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Sent] buffer info to client " << c);
	}

	// Handle Index requests using multiple threads
	for (size_t i = 0; i < config::SERVER_THREADS_CNT; i++) {
		threadsActiveStateFlag[i] = true;
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Starting thread #" << i << " for handling index requests");
		indexHandlerThreads.push_back(std::thread(&TPCC::TPCCServer::handleIndexRequests, this, &threadsActiveStateFlag[i]));
	}
	for (size_t i = 0; i < config::SERVER_THREADS_CNT; i++) {
		indexHandlerThreads[i].join();
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Thread " << i << " is finished");
	}
	PRINT_COUT(CLASS_NAME, __func__, "[Info] Update handler is finished");


	// Gracefully destroy the connections
	for (size_t c = 0; c < clientsCnt_; c++) {
		TEST_NZ (utils::sock_sync (clientCtxs[c]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Client " << c << " notified it's finished");
		delete clientCtxs[c];
	}
	PRINT_COUT(CLASS_NAME, __func__, "[Info] Server's ready to gracefully get destroyed");
}

void TPCC::TPCCServer::handleIndexRequests(bool *isThreadInActiveState) {
	int ret;
	while (liveClientCnt_ > 0 && *isThreadInActiveState){
		uint32_t qpNum = -1;
		ret = RDMACommon::poll_completion(context_->getRecvCq(), qpNum, isThreadInActiveState);
		if (ret < 0){
			utils::die("Cause of Error", __FILE__, __LINE__);
		}
		else if (ret == 1){
			DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] The thread is signaled to stop");
			break;
		}

		size_t clientIndex =  qpNum_to_clientIndex_map[qpNum];	// client index is not the same as clientID. it is simply the index of the client's queue pair in clientCtxs vector.

		TPCC::IndexRequestMessage *req = clientCtxs[clientIndex]->getIndexRequestMessage()->getRegion();

		primitive::client_id_t clientID = req->clientID;
		(void) clientID;	// to avoid unused variable warning

		if (req->operationType == TPCC::IndexRequestMessage::TERMINATE){
			DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Recv] Index request from client " << (int)clientID  << " :: TERMINATE");
			std::lock_guard<std::mutex> lock(liveClientCntLock_);
			liveClientCnt_--;

			// check if there is any more live clients. If not, signal other threads to stop
			if (liveClientCnt_ == 0){
				DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] TERMINATE was for the last client. Signaling other threads to stop");
				for (size_t i = 0; i < config::SERVER_THREADS_CNT; i++)
					threadsActiveStateFlag[i] = false;
				break;
			}
		}
		else{
			ibv_mr		*resRDMAHandler;
			uintptr_t	resPointer;
			uint32_t 	resSize;

			if (req->indexType == TPCC::IndexRequestMessage::IndexType::REGISTER_ORDER){
				TPCC::IndexResponseMessage *res = clientCtxs[clientIndex]->getIndexResponseMessage()->getRegion();
				db->handleRegisterOrderIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(IndexResponseMessage);
			}
			else if (req->indexType == TPCC::IndexRequestMessage::IndexType::LARGEST_ORDER_FOR_CUSTOMER_INDEX){
				TPCC::LargestOrderForCustomerIndexRespMsg *res = clientCtxs[clientIndex]->getLargestOrderForCustomerIndexResponseMessage()->getRegion();
				db->handleLargestOrderIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getLargestOrderForCustomerIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(LargestOrderForCustomerIndexRespMsg);
			}
			else if (req->indexType == TPCC::IndexRequestMessage::IndexType::CUSTOMER_LAST_NAME_INDEX){
				TPCC::CustomerNameIndexRespMsg *res = clientCtxs[clientIndex]->getCustomerNameIndexResponseMessage()->getRegion();
				db->handleCustomerNameIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getCustomerNameIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(CustomerNameIndexRespMsg);
			}
			else if (req->indexType == TPCC::IndexRequestMessage::IndexType::ITEMS_FOR_LAST_20_ORDERS){
				TPCC::Last20OrdersIndexResMsg *res = clientCtxs[clientIndex]->getLast20OrdersIndexResponseMessage()->getRegion();
				db->handleLast20OrdersIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getLast20OrdersIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(Last20OrdersIndexResMsg);
			}
			else if (req->indexType == TPCC::IndexRequestMessage::IndexType::OLDEST_UNDELIVERED_ORDER){
				TPCC::OldestUndeliveredOrderIndexResMsg *res = clientCtxs[clientIndex]->getOldestUndeliveredOrderIndexResponseMessage()->getRegion();
				db->handleOldestUndeliveredOrderIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getOldestUndeliveredOrderIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(OldestUndeliveredOrderIndexResMsg);
			}
			else if (req->indexType == TPCC::IndexRequestMessage::IndexType::REGISTER_DELIVERY){
				TPCC::IndexResponseMessage *res = clientCtxs[clientIndex]->getIndexResponseMessage()->getRegion();
				db->handleRegisterDeliveryIndexRequest(*req, *res);
				resRDMAHandler	= clientCtxs[clientIndex]->getIndexResponseMessage()->getRDMAHandler();
				resPointer		=  (uintptr_t)res;
				resSize 		= sizeof(IndexResponseMessage);
			}
			else {
				PRINT_CERR(CLASS_NAME, __func__, "[ERROR] Unknown index message type");
				exit(-1);
			}

			// to avoid race condition, post the next indexRequest message before sending the response
			TEST_NZ (RDMACommon::post_RECEIVE (
					clientCtxs[clientIndex ]->getQP(),
					clientCtxs[clientIndex ]->getIndexRequestMessage()->getRDMAHandler(),
					(uintptr_t)clientCtxs[clientIndex ]->getIndexRequestMessage()->getRegion(),
					sizeof(IndexRequestMessage)));

			TEST_NZ (RDMACommon::post_SEND(
					clientCtxs[clientIndex ]->getQP(),
					resRDMAHandler,
					resPointer,
					resSize,
					true));

			DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Send] Index response to client " << (int)clientID);
			TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
		}
	}
}

TPCC::TPCCServer::~TPCCServer() {
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete memoryKeysMessage_;
	delete db;
	delete context_;

	// if os_ == &std::cout, deleting os_ will result in core dumped
	if (os_ != &std::cout)
		delete os_;
}
