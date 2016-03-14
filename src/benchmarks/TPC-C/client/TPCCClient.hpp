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
#include "NewOrderLocalMemory.hpp"
#include "DBExecutor.hpp"
#include "../../../oracle/OracleContext.hpp"
#include "../../../../config.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../random/randomgenerator.hpp"
#include "../../../basic-types/Error.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <ctime>	// for std::time and time_t
#include <sstream>

namespace TPCC{
struct NewOrderItem {
	uint32_t I_ID;
	uint16_t OL_SUPPLY_W_ID;
	uint8_t OL_QUANTITY;
};

struct Cart{
	uint16_t wID;
	uint8_t dID;
	uint32_t cID;
	std::vector<NewOrderItem> items;

	friend std::ostream& operator<<(std::ostream& os, const Cart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | cID:" << (int)c.cID << ". --- Items (iID, suppWID):";
		for(auto const& value: c.items)
			os << " {i: " << value.I_ID << " | W: " << value.OL_SUPPLY_W_ID << "}, ";
		return os;
	}
};

struct TransactionResult {
	enum Result {
		COMMITTED,
		ABORTED
	} result;
	enum Reason {
		SUCCESS,
		INCONSISTENT_SNAPSHOT,
		UNSUCCESSFUL_LOCK
	} reason;
	primitive::timestamp_t cts;
};

class TPCCClient {
private:
	primitive::client_id_t	clientID_;
	const unsigned instanceNum_;
	const uint8_t ibPort_;
	size_t	clientCnt_;
	std::vector<ServerContext*> dsCtx_;
	NewOrderLocalMemory* localMemory_;
	TPCC::RealRandomGenerator random_;
	RDMAContext *context_;
	OracleContext *oracleContext_;
	SessionState *sessionState_;
	DBExecutor executor_;
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;
	uint64_t nextOrderID_;
	uint64_t nextNewOrderID_;
	uint64_t nextOrderLineID_;


	TPCC::TransactionResult doNewOrder();
	Cart buildCart();
	ServerContext* getServerContext(uint16_t wID);
	bool isRecordAccessible(Timestamp &ts);
	primitive::timestamp_t getNewCommitTimestamp();
	std::string pointer_to_string(Timestamp* ts) const;


public:
	TPCCClient(unsigned instance_num, uint16_t homeWarehouseID, uint8_t ibPort);
	~TPCCClient();

	TPCCClient& operator=(const TPCCClient&) = delete;	// Disallow copying
	TPCCClient(const TPCCClient&) = delete;				// Disallow copying
};
}	// namespace TPCC
#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_ */
