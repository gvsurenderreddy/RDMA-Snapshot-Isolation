/*
 * OracleReader.cpp
 *
 *  Created on: May 6, 2016
 *      Author: erfanz
 */

#include "OracleReader.hpp"
#include "../../../util/utils.hpp"
#include "../../../../config.hpp"
#include <fstream>	// std::ofstream
#include <unistd.h>	// usleep

namespace TPCC {

#define CLASS_NAME "OracleReader"

OracleReader::OracleReader(unsigned instanceID, uint32_t clientGroupSize, uint8_t ibPort)
: instanceID_(instanceID),
  liveClientCnt_(clientGroupSize){

	if (config::Output::FILE == DEBUG_OUTPUT) {
		std::string filename = std::string(config::LOG_FOLDER) + "/oraclereader_" + std::to_string(instanceID_) + ".log";
		os_ = new std::ofstream (filename, std::ofstream::out);
	}
	else os_ = &std::cout;

	context_ = new RDMAContext(*os_, ibPort);
}

RDMARegion<primitive::timestamp_t> * OracleReader::getTimestampVectorRegion() const{
	return localTimestampVector_;
}


void OracleReader::informAboutFinishing(){
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] One client is finished");
	std::lock_guard<std::mutex> lock(mutex_);
	liveClientCnt_--;
}

void OracleReader::connectToOracle(){
	int sockfd;
	TEST_NZ (utils::establish_tcp_connection(config::TIMESTAMP_SERVER_ADDR, config::TIMESTAMP_SERVER_PORT, &sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Oracle");
	oracleContext_ = new OracleContext(std::cout, sockfd, config::TIMESTAMP_SERVER_PORT, config::TIMESTAMP_SERVER_IB_PORT, *context_);

	char type = 'i';
	TEST_NZ(utils::sock_write(sockfd, &type, sizeof(char)));

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

	clientCnt_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_cnt;
	localTimestampVector_	= new RDMARegion<primitive::timestamp_t>(clientCnt_, *context_, IBV_ACCESS_LOCAL_WRITE);
}

void OracleReader::start(){
	connectToOracle();
	MemoryHandler<primitive::timestamp_t> &remoteMH = oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector;
	primitive::timestamp_t *lookupAddress = (primitive::timestamp_t*)remoteMH.rdmaHandler_.addr;
	ibv_qp* qp = oracleContext_->getQP();
	uint32_t size = (uint32_t)(remoteMH.regionSize_ * sizeof(primitive::timestamp_t));

	bool signaled = false;
	uint64_t counter = 0;
	while (liveClientCnt_ > 0) {
		if (counter % 100 == 0) signaled = true;
		else signaled = false;
		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(
				IBV_WR_RDMA_READ,
				qp,
				localTimestampVector_->getRDMAHandler(),
				(uintptr_t)localTimestampVector_->getRegion(),
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				signaled));

		if (signaled)
			TEST_NZ(RDMACommon::poll_completion(context_->getSendCq()));
		usleep(config::ORACLE_READER_SLEEP_TIME);

		counter++;
		if (counter > 1000 * 1000 * 1000)
			counter = 0;
	}

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] No more clients");
	disconnectFromOracle();
}

void OracleReader::disconnectFromOracle(){
	// Closing socket connection
	TEST_NZ (utils::sock_sync (oracleContext_->getSockFd()));	// just send a dummy char back and forth
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] disconnected from Oracle");
}

OracleReader::~OracleReader() {
	DEBUG_WRITE(*os_, CLASS_NAME, __func__, "[Info] Destructor called");

	delete localTimestampVector_;
	delete oracleContext_;
	delete context_;

	// if os_ == &std::cout, deleting os_ will result in core dumped
	if (os_ != &std::cout)
		delete os_;
}

} /* namespace TPCC */
