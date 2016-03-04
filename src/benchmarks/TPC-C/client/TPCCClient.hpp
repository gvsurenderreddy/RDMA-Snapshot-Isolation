/*
 * TPCCClient.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_

#include "../../../../config.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "SessionState.hpp"
#include "ServerContext.hpp"
#include "OracleContext.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../random/randomgenerator.hpp"
#include "../../../basic-types/Error.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <ctime>	// for std::time and time_t
#include "NewOrderLocalMemory.hpp"
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
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | cID:" << (int)c.cID << std::endl;
		for(auto const& value: c.items)
			os << " -- iID: " << value.I_ID << " | olSupplyWID: " << value.OL_SUPPLY_W_ID << " | olQuantity: " << (int)value.OL_QUANTITY << std::endl;
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
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;



	TPCC::TransactionResult doNewOrder();
	Cart buildCart();
	ServerContext* getServerContext(uint16_t wID);
	uint16_t getWarehouseOffsetOnServer(uint16_t wID);
	bool isRecordAccessible(Timestamp &ts);
	void getReadTimestamp();
	void submitResult(TPCC::TransactionResult trxResult);
	primitive::timestamp_t getNewCommitTimestamp();
	void retrieveWarehouseTax(uint16_t wID);
	void retrieveDistrictTax(uint16_t wID, uint8_t dID);
	uint32_t retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID);
	TPCC::CustomerVersion* getCustomerInformation(uint16_t wID, uint8_t dID, uint32_t cID);
	TPCC::ItemVersion *retrieveItem(uint8_t olNumber, uint32_t iID, uint16_t wID);
	TPCC::StockVersion* retrieveStock(uint8_t olNumber, uint32_t iID, uint16_t wID);
	void retrieveStockPointerList(uint8_t olNumber, uint32_t iID, uint16_t wID, bool signaled);
	void updateStockPointers(uint8_t olNumber, StockVersion *oldHead);
	void updateStockOlderVersions(uint8_t olNumber, StockVersion *oldHead);

	TPCC::OrderVersion* insertIntoOrder(uint32_t oID, Cart &cart, time_t timer, Timestamp writeTimestamp);
	TPCC::NewOrderVersion* insertIntoNewOrder(uint32_t oID, uint16_t wID, uint8_t dID, Timestamp writeTimestamp);
	error::ErrorType updateStock(uint8_t olNumber, TPCC::StockVersion *stockV);
	TPCC::OrderLineVersion* insertIntoOrderLine(uint8_t olNumber, uint32_t oID, Cart &cart, NewOrderItem &newOrderItem, TPCC::ItemVersion *itemV, TPCC::StockVersion *stockV, Timestamp &ts, bool signaled);
	void lockStock(uint8_t olNumber, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS);
	void revertStockLock(uint8_t olNumber, uint32_t iID, uint16_t wID);
	std::string pointer_to_string(Timestamp* ts) const;




public:
	TPCCClient(unsigned instance_num, uint16_t homeWarehouseID, uint8_t ibPort);
	~TPCCClient();

	TPCCClient& operator=(const TPCCClient&) = delete;	// Disallow copying
	TPCCClient(const TPCCClient&) = delete;				// Disallow copying
};
}	// namespace TPCC
#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_ */
