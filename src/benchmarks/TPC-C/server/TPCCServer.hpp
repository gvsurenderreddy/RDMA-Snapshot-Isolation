/*
 * TPCCServer.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_SERVER_TPCCSERVER_HPP_
#define SRC_BENCHMARKS_TPC_C_SERVER_TPCCSERVER_HPP_

#include "ClientContext.hpp"
#include "../tables/TPCCDB.hpp"
#include "ServerMemoryKeys.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../../../util/RDMACommon.hpp"

#include <unistd.h>	// for close()
#include <infiniband/verbs.h>	// for ibv_qp
#include <vector>	// for std::vector

namespace TPCC{
class TPCCServer {
public:
	TPCCServer(uint32_t serverNum, unsigned instanceNum, uint32_t clientsCnt);
	virtual ~TPCCServer();

private:
	const uint32_t serverNum_;
	const unsigned instanceNum_;
	const uint32_t clientsCnt_;
	int server_sockfd_;
	uint16_t	tcp_port_;
	uint8_t	ib_port_;
	TPCC::TPCCDB	*db;
	RDMAContext *context_;


	std::vector<ClientContext*> clientCtxs;

	RDMARegion<ServerMemoryKeys> *memoryKeysMessage_;
};
}

#endif /* SRC_BENCHMARKS_TPC_C_SERVER_TPCCSERVER_HPP_ */
