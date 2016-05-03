/*
 * TPCCClient.cpp
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#include "TPCCClient.hpp"
#include "../../../rdma-region/RDMACommon.hpp"
#include "../../../util/utils.hpp"
#include "../tables/TPCCUtil.hpp"
#include "../tables/WarehouseTable.hpp"
#include "queries/new-order/NewOrderTransaction.hpp"
#include "queries/payment/PaymentTransaction.hpp"
#include "queries/order-status/OrderStatusTransaction.hpp"
#include "queries/stock-level/StockLevelTransaction.hpp"
#include "queries/delivery/DeliveryTransaction.hpp"
#include <infiniband/verbs.h>
#include <string>
#include <vector>
#include <time.h>		// for struct timespec
#include <cassert>
#include <memory>	//std::unique_ptr
#include <fstream>      // std::ofstream
#include <iostream>
#include <unistd.h>


#define CLASS_NAME "TPCCClient"

namespace TPCC {

TPCCClient::TPCCClient(unsigned instanceNum, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort){

	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed
	TPCC::NURandC cLoad = TPCC::NURandC::makeRandom(random_);
	random_.setC(cLoad);

	context_ 		= new RDMAContext(std::cout, ibPort_);
	uint16_t homeWarehouseID = (uint16_t)(instanceNum * config::tpcc_settings::WAREHOUSE_PER_SERVER + random_.number(0, config::tpcc_settings::WAREHOUSE_PER_SERVER - 1));
	uint8_t homeDistrictID = (uint8_t)random_.number(0, config::tpcc_settings::DISTRICT_PER_WAREHOUSE - 1);

	sessionState_ 	= new SessionState(homeWarehouseID, homeDistrictID, (primitive::timestamp_t) 1ULL);
}

void TPCCClient::start(){
	// ************************************************
	//	Connect to Oracle
	// ************************************************
	int sockfd;
	TEST_NZ (utils::establish_tcp_connection(config::TIMESTAMP_SERVER_ADDR, config::TIMESTAMP_SERVER_PORT, &sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Oracle");
	oracleContext_ = new OracleContext(std::cout, sockfd, config::TIMESTAMP_SERVER_PORT, config::TIMESTAMP_SERVER_IB_PORT, *context_);

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

	if (config::Output::FILE == DEBUG_OUTPUT) {
		std::string filename = std::string(config::LOG_FOLDER) + "/client_" + std::to_string(clientID_) + ".log";
		os_ = new std::ofstream (filename, std::ofstream::out);
	}
	else os_ = &std::cout;

	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Client is assigned ID = " << clientID_ << " out of " << clientCnt_ << " clients");

	localTimestampVector_	= new RDMARegion<primitive::timestamp_t>(clientCnt_, *context_, IBV_ACCESS_LOCAL_WRITE);



	// ************************************************
	//	Connect to Servers
	// ************************************************
	for (size_t i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::establish_tcp_connection(config::SERVER_ADDR[i], config::TCP_PORT[i], &sockfd));

		// build server context
		dsCtx_.push_back(new ServerContext(*os_, sockfd, config::SERVER_ADDR[i], config::TCP_PORT[i], config::IB_PORT[i], *context_));

		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Connection established to server " << i);


		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getRemoteMemoryKeys()->getRDMAHandler(),
				(uintptr_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegion(),
				(uint32_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegionSizeInByte());

		dsCtx_[i]->activateQueuePair(*context_);
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] QPed to server " << i);

		TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));

		dsCtx_[i]->setInstanceNum(dsCtx_[i]->getRemoteMemoryKeys()->getRegion()->serverInstanceNum);
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Recv] Buffers info from server " << i);
	}

	TPCC::DBExecutor executor_(*os_, dsCtx_, instanceNum_, context_->getSendCq(), context_->getRecvCq());
	std::vector<std::unique_ptr<TPCC::BaseTransaction> > trxs;

	// ************************************************
	//	Setup the ReplicationClient
	// ************************************************
	if (config::recovery_settings::RECOVERY_ENABLED){
		std::vector<std::pair<MemoryHandler<char>, ibv_qp*> > logServers;
		for (unsigned i = 0; i < config::recovery_settings::LOG_REPLICATION_DEGREE; i++){
			size_t serverID = (size_t)((clientID_ % config::SERVER_CNT + i) % config::SERVER_CNT);
			logServers.push_back(std::make_pair(dsCtx_[serverID]->getRemoteMemoryKeys()->getRegion()->logBuffer, dsCtx_[serverID]->getQP()));
		}
		recoveryClient_ = new RecoveryClient(*os_, clientID_, clientCnt_, logServers, *context_);
	}


	// ************************************************
	//	Preparing the transactions
	// ************************************************
	trxs.push_back(std::unique_ptr<TPCC::BaseTransaction>(new NewOrderTransaction (*os_, executor_, clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_, recoveryClient_)));
	trxs.push_back(std::unique_ptr<TPCC::BaseTransaction>(new PaymentTransaction (*os_, executor_, clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_, recoveryClient_)));
	trxs.push_back(std::unique_ptr<TPCC::BaseTransaction>(new OrderStatusTransaction (*os_, executor_, clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_, recoveryClient_)));
	trxs.push_back(std::unique_ptr<TPCC::BaseTransaction>(new DeliveryTransaction (*os_, executor_, clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_, recoveryClient_)));
	trxs.push_back(std::unique_ptr<TPCC::BaseTransaction>(new StockLevelTransaction (*os_, executor_, clientID_, clientCnt_, dsCtx_, sessionState_, &random_, context_, oracleContext_, localTimestampVector_, recoveryClient_)));


	struct timespec trxBeginTime, trxFinishTime;
	std::unordered_map<std::string, unsigned> abortCnt;
	std::unordered_map<std::string, unsigned> abortDueToInconsistentSnapshot;
	std::unordered_map<std::string, unsigned> abortDueToUnsuccessfulLock;
	std::unordered_map<std::string, double> elapsedMicroSec;
	std::unordered_map<std::string, unsigned> executedTrxCnt;
	std::unordered_map<std::string, double> elapsedMicroSecInExecution;
	std::unordered_map<std::string, double> elapsedMicroSecInIndex;
	std::unordered_map<std::string, double> elapsedMicroSecInCheckingVersions;
	std::unordered_map<std::string, double> elapsedMicroSecInLocking;
	std::unordered_map<std::string, double> elapsedMicroSecInUpdatingRecords;
	std::unordered_map<std::string, double> elapsedMicroSecInCommitingSnapshot;

	for (auto& trx: trxs){
		std::string s = trx->getTransactionName();
		abortCnt[s] = 0;
		abortDueToInconsistentSnapshot[s] = 0;
		abortDueToUnsuccessfulLock[s] = 0;
		elapsedMicroSec[s] = 0.0;
		elapsedMicroSecInExecution[s] = 0.0;
		elapsedMicroSecInIndex[s] = 0.0;
		elapsedMicroSecInCheckingVersions[s] = 0.0;
		elapsedMicroSecInLocking[s] = 0.0;
		elapsedMicroSecInUpdatingRecords[s] = 0.0;
		elapsedMicroSecInCommitingSnapshot[s] = 0.0;
		executedTrxCnt[s] = 0;
	}


	// ************************************************
	//	Running the transactions
	// ************************************************
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Starting transactions ");
	for (unsigned t = 0; t < config::tpcc_settings::TRANSACTION_CNT; t++){
		// decided which transaction to execute
		BaseTransaction *trx;

		int r = random_.number(1, 100);
		double d = (double) r / 100;
		for (size_t i = 0; i < config::tpcc_settings::TRANSACTION_MIX_RATIOS.size(); i++){
			if (d <= config::tpcc_settings::TRANSACTION_MIX_RATIOS.at(i)
					|| i == (config::tpcc_settings::TRANSACTION_MIX_RATIOS.size() - 1)	// this line is for dealing with double precision issues
			){
				trx = trxs[i].get();
				break;
			}
			else d -= config::tpcc_settings::TRANSACTION_MIX_RATIOS.at(i);
		}

		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] --------------- Transaction " << t << " (" << trx->getTransactionName() << ") --------------");
		clock_gettime(CLOCK_REALTIME, &trxBeginTime);
		TransactionResult trxResult = trx->doOne();
		clock_gettime(CLOCK_REALTIME, &trxFinishTime);

		std::string trxName = trx->getTransactionName();
		executedTrxCnt[trxName]++;
		elapsedMicroSecInExecution[trxName] = trxResult.statistics.executionPhaseMicroSec;
		elapsedMicroSecInIndex[trxName] = trxResult.statistics.indexElapsedMicroSec;
		elapsedMicroSecInCheckingVersions[trxName] = trxResult.statistics.checkVersionsPhaseMicroSec;
		elapsedMicroSecInLocking[trxName] = trxResult.statistics.lockPhaseMicroSec;
		elapsedMicroSecInUpdatingRecords[trxName] = trxResult.statistics.updatePhaseMicroSec;
		elapsedMicroSecInCommitingSnapshot[trxName] = trxResult.statistics.commitSnapshotMicroSec;
		elapsedMicroSec[trxName] +=  ( (double)( trxFinishTime.tv_sec - trxBeginTime.tv_sec ) * 1E9 + (double)( trxFinishTime.tv_nsec - trxBeginTime.tv_nsec ) ) / 1000;

		if (trxResult.result == TransactionResult::Result::ABORTED){
			abortCnt[trxName]++;
			if (trxResult.reason == TransactionResult::Reason::INCONSISTENT_SNAPSHOT)
				abortDueToInconsistentSnapshot[trxName]++;
			else if (trxResult.reason == TransactionResult::Reason::UNSUCCESSFUL_LOCK)
				abortDueToUnsuccessfulLock[trxName]++;
		}
	}

	// ************************************************
	//	Printing the statistics
	// ************************************************
	std::cout << std::endl;
	for (auto& trx: trxs){
		std::string n = trx->getTransactionName();
		if (executedTrxCnt[n] == 0)
			continue;
		double abortRate = (double)abortCnt[n] / executedTrxCnt[n];
		double inconsistentSnapshotRatio = (abortCnt[n]==0) ? 0 : (double)abortDueToInconsistentSnapshot[n]/abortCnt[n];
		double unsuccessfulLockRatio = (abortCnt[n]==0) ? 0 : (double)abortDueToUnsuccessfulLock[n]/abortCnt[n];
		unsigned committedCnt = executedTrxCnt[n] - abortCnt[n];
		double trxsPerSec = (double)(committedCnt / (double)(elapsedMicroSec[n] / (1000 * 1000) ));

		std::cout << "[Stat] (Trx: " << n << ") committed: " << committedCnt << ", aborted: " << abortCnt[n] << ". abort rate:	" << abortRate << std::endl;
		std::cout << "[Stat] (Trx: " << n << ") Avg abort type I (stale snapshot) ratio	" << inconsistentSnapshotRatio << std::endl;
		std::cout << "[Stat] (Trx: " << n << ") Avg abort type II (failed locks) ratio	" << unsuccessfulLockRatio << std::endl;
		std::cout << "[Stat] (Trx: " << n << ") Committed Transactions/sec:	" <<  trxsPerSec << std::endl;

		if (n == trxs[0]->getTransactionName()) {
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in execution (usec):	" << elapsedMicroSecInExecution[n] << std::endl;
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in index (usec):	" << elapsedMicroSecInIndex[n] << std::endl;
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in version check (usec):	" << elapsedMicroSecInCheckingVersions[n] << std::endl;
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in locking (usec):	" << elapsedMicroSecInLocking[n] << std::endl;
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in updating records (usec):	" << elapsedMicroSecInUpdatingRecords[n] << std::endl;
			std::cout << "[Stat] (Trx: " << n << ") Avg elapsed time in committing snapshot (usec):	" << elapsedMicroSecInCommitingSnapshot[n] << std::endl;
		}

	}

	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Client " << (int)clientID_ << " is done, and is ready to destroy its resources!");
	for (size_t i = 0; i < config::SERVER_CNT; i++){
		TPCC::IndexRequestMessage *req = dsCtx_[i]->getIndexRequestMessage()->getRegion();
		req->clientID = clientID_;
		req->operationType = TPCC::IndexRequestMessage::OperationType::TERMINATE;
		RDMACommon::post_SEND(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getIndexRequestMessage()->getRDMAHandler(),
				(uintptr_t)req,
				(uint32_t)dsCtx_[i]->getIndexRequestMessage()->getRegionSizeInByte(),
				true);
		TEST_NZ(RDMACommon::poll_completion(context_->getSendCq()));
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Client " << (int)clientID_ << " terminates the index connection");


		TEST_NZ (utils::sock_sync (dsCtx_[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Client " << (int)clientID_ << " notified server " << i << " it's done");

		delete dsCtx_[i];
	}

	TEST_NZ (utils::sock_sync (oracleContext_->getSockFd()));	// just send a dummy char back and forth
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Conn] Client " << (int)clientID_ << " notified Oracle it's done");
}

TPCCClient::~TPCCClient(){
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Destructor called");
	delete localTimestampVector_;
	delete oracleContext_;
	if (config::recovery_settings::RECOVERY_ENABLED) delete recoveryClient_;
	delete context_;
	delete sessionState_;

	// if os_ == &std::cout, deleting os_ will result in core dumped
	if (os_ != &std::cout)
		delete os_;
}

}	// namespace TPCC
