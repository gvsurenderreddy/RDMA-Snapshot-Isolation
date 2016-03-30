/*
 * TPCCClient.cpp
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "TPCCClient.hpp"
#include "../../../util/RDMACommon.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include "../tables/WarehouseTable.hpp"
#include <infiniband/verbs.h>
#include <string>
#include <vector>
#include <time.h>		// for struct timespec
#include <cassert>

#define CLASS_NAME "TPCCClient"

TPCC::TPCCClient::TPCCClient(unsigned instanceNum, uint16_t homeWarehouseID, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort){
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed
	//zipf_initialize(config::SKEWNESS_IN_ITEM_ACCESS, config::ITEM_PER_SERVER);
	// nextEpoch_ = (primitive::timestamp_t) 1ULL;

	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random_);
	random_.setC(cLoad);

	context_ 		= new RDMAContext(ibPort_);
	sessionState_ 	= new SessionState(homeWarehouseID, (primitive::timestamp_t) 1ULL);

	// ************************************************
	//	Connect to Oracle
	// ************************************************
	int sockfd;
	TEST_NZ (utils::establish_tcp_connection(config::TIMESTAMP_SERVER_ADDR, config::TIMESTAMP_SERVER_PORT, &sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Oracle");
	oracleContext_ = new OracleContext(sockfd, config::TIMESTAMP_SERVER_PORT, config::TIMESTAMP_SERVER_IB_PORT, *context_);

	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(
			oracleContext_->getQP(),
			oracleContext_->getRemoteMemoryKeys()->getRDMAHandler(),
			(uintptr_t)oracleContext_->getRemoteMemoryKeys()->getRegion(),
			(uint32_t)oracleContext_->getRemoteMemoryKeys()->getRegionSizeInByte());


	oracleContext_->activateQueuePair(*context_);
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to Oracle");

	TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from Oracle");

	// Set client ID and client cnt
	clientID_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_id;
	clientCnt_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_cnt;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is assigned ID = " << clientID_ << " out of " << clientCnt_ << " clients");

	localTimestampVector_	= new RDMARegion<primitive::timestamp_t>(clientCnt_, *context_, IBV_ACCESS_LOCAL_WRITE);



	// ************************************************
	//	Connect to Servers
	// ************************************************
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::establish_tcp_connection(config::SERVER_ADDR[i], config::TCP_PORT[i], &sockfd));

		// build server context
		dsCtx_.push_back(new ServerContext(sockfd, config::SERVER_ADDR[i], config::TCP_PORT[i], config::IB_PORT[i], instanceNum, *context_));

		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server " << i);


		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getRemoteMemoryKeys()->getRDMAHandler(),
				(uintptr_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegion(),
				(uint32_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegionSizeInByte());

		dsCtx_[i]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server " << i);

		TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from server " << i);
	}

	NewOrderTransaction newOrderTrx_ (clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_);
	PaymentTransaction paymentTrx_ (clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_);



	int abortCnt = 0;
	int committedCnt = 0;
	int abortDueToInconsistentSnapshot = 0;
	int abortDueToUnsuccessfulLock = 0;
	struct timespec firstRequestTime, lastRequestTime;

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Starting transactions ");
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);

	for (int i=0; i < config::tpcc_settings::TRANSACTION_CNT; i++){
		DEBUG_COUT(CLASS_NAME, __func__, "--------------- [Info] Transaction " << i << ": --------------");
		//TransactionResult trxResult = newOrderTrx_.doOne();
		TransactionResult trxResult = paymentTrx_.doOne();
		if (trxResult.result == TransactionResult::Result::ABORTED){
			abortCnt++;
			if (trxResult.reason == TransactionResult::Reason::INCONSISTENT_SNAPSHOT)
				abortDueToInconsistentSnapshot++;
			else if (trxResult.reason == TransactionResult::Reason::UNSUCCESSFUL_LOCK)
				abortDueToUnsuccessfulLock++;
		}
		else committedCnt++;
	}
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);

	double microElapsedTime = ( (double)( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + (double)( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	double abort_rate = 1 - (double)committedCnt / config::tpcc_settings::TRANSACTION_CNT;
	double inconsistentSnapshotRatio = (abortCnt==0) ? 0 : (double)abortDueToInconsistentSnapshot/abortCnt;
	double unsuccessfulLockRatio = (abortCnt==0) ? 0 : (double)abortDueToUnsuccessfulLock/abortCnt;
	double trxsPerSec = (double)(committedCnt / (double)(microElapsedTime / (1000 * 1000) ));

	std::cout << "[Stat] " << committedCnt << " committed, " << abortCnt << " aborted. abort rate:	" << abort_rate << std::endl;
	std::cout << "[Stat] Avg abort type I (snapshot) ratio	" << inconsistentSnapshotRatio << std::endl;
	std::cout << "[Stat] Avg abort type II (lock) ratio	" << unsuccessfulLockRatio << std::endl;
	std::cout << "[Stat] Committed Transactions/sec:	" <<  trxsPerSec << std::endl;

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client " << (int)clientID_ << " is done, and is ready to destroy its resources!");
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::sock_sync (dsCtx_[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Client " << (int)clientID_ << " notified server " << i << " it's done");
		delete dsCtx_[i];
	}

	TEST_NZ (utils::sock_sync (oracleContext_->getSockFd()));	// just send a dummy char back and forth
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Client " << (int)clientID_ << " notified Oracle it's done");
}

TPCC::TPCCClient::~TPCCClient(){
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Destructor called");
	delete localTimestampVector_;
	delete oracleContext_;
	delete context_;
	delete sessionState_;
}
