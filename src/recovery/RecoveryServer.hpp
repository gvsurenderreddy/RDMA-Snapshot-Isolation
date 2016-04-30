/*
 * RecoverySERVER.hpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#ifndef SRC_RECOVERY_RECOVERYSERVER_HPP_
#define SRC_RECOVERY_RECOVERYSERVER_HPP_

#include "LogBuffer.hpp"
#include "../basic-types/PrimitiveTypes.hpp"
#include "../rdma-region/MemoryHandler.hpp"
#include "../rdma-region/RDMAContext.hpp"
#include "../rdma-region/RDMARegion.hpp"
#include <vector>

class RecoveryServer {
private:
	std::ostream &os_;
	size_t		clientsCnt_;

public:
	std::vector<LogBuffer *> logBuffers;
	RecoveryServer(std::ostream &os, size_t clientsCnt, RDMAContext &context);
	~RecoveryServer();
	MemoryHandler<char> getMemoryHandler(primitive::client_id_t clientID);
};

#endif /* SRC_RECOVERY_RECOVERYSERVER_HPP_ */
