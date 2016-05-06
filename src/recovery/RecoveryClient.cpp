/*
 * RecoveryManagerClient.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#include "RecoveryClient.hpp"
#include "LogBuffer.hpp"
#include <iostream>		// std::cout
#include <cassert>		// assert()
#include <cstring>		// std::memcpy


#define CLASS_NAME "RecoveryClient"

RecoveryClient::RecoveryClient(std::ostream &os, primitive::client_id_t clientID, size_t clientsCnt, std::vector<std::pair<MemoryHandler<char>, ibv_qp*> > logServers, RDMAContext &context)
: os_(os),
  clientID_(clientID),
  clientsCnt_(clientsCnt),
  nextLogEntryIndex_(0),
  logServers_(logServers),
  context_(context){

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] RecoveryClient created");
	assert(logServers_.size() == config::recovery_settings::LOG_REPLICATION_DEGREE);
	entrySize_ = (uint32_t)LogBuffer::getEntrySize(clientsCnt_);
	localRegion_ = new RDMARegion<char>(entrySize_, context, IBV_ACCESS_LOCAL_WRITE);
}

void RecoveryClient::writeCommandToLog(RDMARegion<primitive::timestamp_t> &timestampVector, const std::set<primitive::client_id_t> &clientsInSnapshot, const char *command, size_t commandSize){
	assert(commandSize < config::recovery_settings::COMMAND_LOG_SIZE);

	// TODO: for config::SNAPSHOT_ACQUISITION_TYPE = ONLY_READ_SET, adjust the logging
	(void)clientsInSnapshot;

	char *buff = localRegion_->getRegion();
	for (size_t i = 0; i < timestampVector.getRegionSize(); i++)
		LogBuffer::serializeUnsignedINT32_t(buff + i * LogBuffer::serializedTimestampCharCnt, timestampVector.getRegion()[i]);

	std::memcpy(buff + timestampVector.getRegionSize() * LogBuffer::serializedTimestampCharCnt, command, commandSize);

	for(auto &server: logServers_) {
		size_t tableOffset = nextLogEntryIndex_ * entrySize_;				// offset of LogEntry in Journal
		char *writeAddress =  (char*)(tableOffset + (uint64_t)server.first.rdmaHandler_.addr);

		if (server.first.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: command = " << command);
			exit(-1);
		}

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				server.second,
				localRegion_->getRDMAHandler(),
				(uintptr_t)localRegion_->getRegion(),
				&(server.first.rdmaHandler_),
				(uintptr_t)writeAddress,
				(uint32_t)commandSize,
				true));
	}

	for(size_t i = 0; i < config::recovery_settings::LOG_REPLICATION_DEGREE; i++)
		TEST_NZ(RDMACommon::poll_completion(context_.getSendCq()));

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Command written to log journal");
}

void RecoveryClient::writeResultToLog(char transactionResult){
	assert(transactionResult == 'A' || transactionResult == 'C');

	size_t offsetOfOutcome = LogBuffer::getOffsetOfTrxOutcome(clientsCnt_);
	localRegion_->getRegion()[offsetOfOutcome] = transactionResult;

	for(auto &server: logServers_) {
		size_t tableOffset = nextLogEntryIndex_ * entrySize_;				// offset of LogEntry in Journal
		char *writeAddress =  (char*)(tableOffset + offsetOfOutcome + (uint64_t)server.first.rdmaHandler_.addr);

		if (server.first.isAddressInRange((uintptr_t)writeAddress) == false){
			PRINT_CERR(CLASS_NAME, __func__, "Parameters causing the error: transaction result = " << transactionResult);
			exit(-1);
		}

		uint32_t size = 1;	// only one 1 char. either C or A
		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				server.second,
				localRegion_->getRDMAHandler(),
				(uintptr_t)&localRegion_->getRegion()[offsetOfOutcome],
				&(server.first.rdmaHandler_),
				(uintptr_t)writeAddress,
				size,
				true));
	}

	for(size_t i = 0; i < config::recovery_settings::LOG_REPLICATION_DEGREE; i++)
		TEST_NZ(RDMACommon::poll_completion(context_.getSendCq()));

	nextLogEntryIndex_ = (size_t) ( (nextLogEntryIndex_ + 1) % config::recovery_settings::ENTRY_PER_LOG_JOURNAL);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Trx outcome written to log journal");
}

RecoveryClient::~RecoveryClient(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete localRegion_;
}
