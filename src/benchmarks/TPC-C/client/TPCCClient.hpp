/*
 * TPCCClient.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_

#include "SessionState.hpp"
#include "ServerContext.hpp"
#include "TransactionResult.hpp"
#include "../../../oracle/OracleContext.hpp"
#include "../../../../config.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../random/randomgenerator.hpp"
#include "queries/NewOrderTransaction.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <ctime>	// for std::time and time_t

namespace TPCC{

class TPCCClient {
private:
	primitive::client_id_t	clientID_;
	const unsigned instanceNum_;
	const uint8_t ibPort_;
	size_t	clientCnt_;
	std::vector<ServerContext*> dsCtx_;
	TPCC::RealRandomGenerator random_;
	RDMAContext *context_;
	OracleContext *oracleContext_;
	SessionState *sessionState_;
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;
	NewOrderTransaction *newOrderTrx_;


public:
	TPCCClient(unsigned instance_num, uint16_t homeWarehouseID, uint8_t ibPort);
	~TPCCClient();

	TPCCClient& operator=(const TPCCClient&) = delete;	// Disallow copying
	TPCCClient(const TPCCClient&) = delete;				// Disallow copying
};
}	// namespace TPCC
#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_ */
