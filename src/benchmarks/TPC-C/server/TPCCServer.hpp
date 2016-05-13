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
#include "../../../rdma-region/RDMACommon.hpp"
#include "../../../recovery/RecoveryServer.hpp"
#include <unistd.h>	// for close()
#include <infiniband/verbs.h>	// for ibv_qp
#include <vector>	// for std::vector
#include <unordered_map>
#include <thread>	// for std::thread
#include <mutex>		// for std::lock_guard


namespace TPCC{
class TPCCServer {
public:
	TPCCServer(uint32_t serverNum, unsigned instanceNum, uint32_t clientsCnt);
	virtual ~TPCCServer();
	void start();

	TPCCServer& operator=(const TPCCServer&) = delete;	// Disallow copying
	TPCCServer(const TPCCServer&) = delete;				// Disallow copying

private:
	const uint32_t serverNum_;
	const unsigned instanceNum_;
	const uint32_t clientsCnt_;
	int server_sockfd_;
	uint16_t	tcp_port_;
	uint8_t	ib_port_;
	TPCC::TPCCDB	*db_;
	RecoveryServer *recoveryServer_;
	RDMAContext *context_;
	std::vector<ClientContext*> clientCtxs;
	RDMARegion<ServerMemoryKeys> *memoryKeysMessage_;
	std::unordered_map<uint32_t, primitive::client_id_t> qpNum_to_clientIndex_map;	// client index is not the same as clientID. it is simply the index of the client's queue pair in clientCtxs vector.
	std::ostream *os_;
	std::vector<std::thread> indexHandlerThreads;
	static bool threadsActiveStateFlag[config::SERVER_THREADS_CNT];
	size_t liveClientCnt_;
	std::mutex liveClientCntLock_;
	void handleIndexRequests(bool *isAlive);
	bool finished;

};
}

#endif /* SRC_BENCHMARKS_TPC_C_SERVER_TPCCSERVER_HPP_ */
