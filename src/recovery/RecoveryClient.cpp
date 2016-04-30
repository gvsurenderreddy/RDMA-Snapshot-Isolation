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

void RecoveryClient::writeToLog(RDMARegion<primitive::timestamp_t> &timestampVector, const char command[config::recovery_settings::COMMAND_LOG_SIZE]){
	char *buff = localRegion_->getRegion();
	for (size_t i = 0; i < timestampVector.getRegionSize(); i++)
		LogBuffer::serializeUnsignedINT32_t(buff + i * LogBuffer::serializedTimestampCharCnt, timestampVector.getRegion()[i]);

	std::memcpy(buff + timestampVector.getRegionSize() * LogBuffer::serializedTimestampCharCnt, command, config::recovery_settings::COMMAND_LOG_SIZE);

	for(auto &server: logServers_) {
		size_t tableOffset = nextLogEntryIndex_ * entrySize_;				// offset of LogEntry in Journal
		char *writeAddress =  (char*)(tableOffset + ((uint64_t)server.first.rdmaHandler_.addr));

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
				entrySize_,
				true));
	}

	for(size_t i = 0; i < config::recovery_settings::LOG_REPLICATION_DEGREE; i++)
		TEST_NZ(RDMACommon::poll_completion(context_.getSendCq()));

	nextLogEntryIndex_ = (size_t) ( (nextLogEntryIndex_ + 1) % config::recovery_settings::ENTRY_PER_LOG_JOURNAL);
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Log written to journal");
}

RecoveryClient::~RecoveryClient(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called");
	delete localRegion_;
}
