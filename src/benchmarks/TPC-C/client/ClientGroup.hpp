/*
 * ClientGroup.hpp
 *
 *  Created on: May 6, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_CLIENTGROUP_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_CLIENTGROUP_HPP_

#include "TPCCClient.hpp"
#include <vector>
#include <thread>

namespace TPCC {

class ClientGroup {
private:
	uint32_t clientsCnt_;
	std::vector<TPCC::TPCCClient *> clients_;
	std::vector<std::thread> clientsThreads_;
	std::thread *oracleReaderThread_;
	OracleReader *oracleReader_;

public:
	ClientGroup(unsigned instanceID, uint32_t clientsCnt, uint16_t homeWarehouseID, size_t ibPortsCnt);
	void start();
	void join();
	virtual ~ClientGroup();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_CLIENTGROUP_HPP_ */
