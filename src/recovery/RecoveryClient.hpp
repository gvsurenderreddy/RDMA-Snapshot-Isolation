/*
 * RecoveryManagerClient.hpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#ifndef SRC_RECOVERY_RECOVERYCLIENT_HPP_
#define SRC_RECOVERY_RECOVERYCLIENT_HPP_

#include "../basic-types/PrimitiveTypes.hpp"
#include "../rdma-region/RDMARegion.hpp"
#include "../rdma-region/RDMAContext.hpp"
#include "../rdma-region/MemoryHandler.hpp"
#include <utility>
#include <string>

class RecoveryClient {
private:
	std::ostream &os_;
	primitive::client_id_t clientID_;
	size_t clientsCnt_;
	uint32_t entrySize_;
	size_t nextLogEntryIndex_;
	std::vector<std::pair<MemoryHandler<char>, ibv_qp*> > logServers_;
	RDMARegion<char> *localRegion_;
	RDMAContext &context_;


public:
	RecoveryClient(std::ostream &os, primitive::client_id_t clientID, size_t clientsCnt, std::vector<std::pair<MemoryHandler<char>, ibv_qp*> > logServers, RDMAContext &context);
	~RecoveryClient();
	void writeCommandToLog(RDMARegion<primitive::timestamp_t> &timestampVector, const char *command, size_t commandSize);
	void writeResultToLog(char transactionResult);
	RecoveryClient& operator=(const RecoveryClient&) = delete;	// Disallow copying
	RecoveryClient(const RecoveryClient&) = delete;				// Disallow copying
};

#endif /* SRC_RECOVERY_RECOVERYCLIENT_HPP_ */
