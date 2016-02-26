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
//#include "TimestampServerContext.hpp"
#include "ServerContext.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../random/randomgenerator.hpp"
#include "../../../basic-types/Error.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <ctime>	// for std::time and time_t


struct NewOrderItem {
    uint32_t I_ID;
    uint32_t OL_SUPPLY_W_ID;
    uint8_t OL_QUANTITY;
};

struct Cart{
	uint16_t wID;
	uint8_t dID;
	uint32_t cID;
	std::vector<NewOrderItem> items;
};

struct SessionState{
	uint32_t				nextCommitOrderID;
	primitive::timestamp_t	nextEpoch_;
};


class TPCCClient {
private:
	primitive::client_id_t	clientID_;
	const unsigned instanceNum_;
	const uint8_t ibPort_;
	size_t	clientCnt_;
	std::vector<ServerContext*> dsCtx_;
	//TimestampServerContext tsCtx_;
	TPCC::RealRandomGenerator random_;
	RDMAContext *context_;
	SessionState sessionState;


	void doNewOrder();
	Cart buildCart();
	ServerContext* getServerContext(uint16_t wID);
	float retrieveWarehouseTax(uint16_t wID);
	float retrieveDistrictTax(uint16_t wID, uint8_t dID);
	uint32_t retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID);

	void getCustomerInformation(uint16_t wID, uint8_t dID, uint32_t cID, char* out_cLast, float &out_cDiscount, char *out_cCredit);
	TPCC::OrderVersion* insertIntoOrder(uint32_t oID, Cart &cart, time_t timer, Timestamp writeTimestamp);
	TPCC::NewOrderVersion* insertIntoNewOrder(uint32_t oID, uint16_t wID, uint8_t dID, Timestamp writeTimestamp);
	TPCC::Item *retrieveItem(uint32_t iID, uint16_t wID);
	TPCC::StockVersion* retrieveStock(uint32_t iID, uint16_t wID);
	error::ErrorType updateStock(TPCC::StockVersion *stockV);
	TPCC::OrderLineVersion* insertIntoOrderLine(uint8_t olNumber, uint32_t oID, Cart &cart, NewOrderItem &newOrderItem, TPCC::Item *item, TPCC::StockVersion *stockV, Timestamp &ts);




public:
	TPCCClient(unsigned instance_num, uint8_t ibPort);
	~TPCCClient();

	TPCCClient& operator=(const TPCCClient&) = delete;	// Disallow copying
	TPCCClient(const TPCCClient&) = delete;				// Disallow copying
};

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_TPCCCLIENT_HPP_ */
