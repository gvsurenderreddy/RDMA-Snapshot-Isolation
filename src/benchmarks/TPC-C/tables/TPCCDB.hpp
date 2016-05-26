/*
 * TPCCDB.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPCCDB_HPP_
#define SRC_BENCHMARKS_TPCCDB_HPP_

#include "CustomerTable.hpp"
#include "ItemTable.hpp"
#include "OrderTable.hpp"
#include "WarehouseTable.hpp"
#include "OrderLineTable.hpp"
#include "NewOrderTable.hpp"
#include "StockTable.hpp"
#include "DistrictTable.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../one-sided-architecture/server/ServerMemoryKeys.hpp"
#include "../random/randomgenerator.hpp"
#include "../one-sided-architecture/index-messages/IndexRequestMessage.hpp"
#include "../one-sided-architecture/index-messages/IndexResponseMessage.hpp"
#include "../one-sided-architecture/index-messages/LargestOrderForCustomerIndexRespMsg.hpp"
#include "../one-sided-architecture/index-messages/CustomerNameIndexRespMsg.hpp"
#include "../one-sided-architecture/index-messages/Last20OrdersIndexResMsg.hpp"
#include "../one-sided-architecture/index-messages/OldestUndeliveredOrderIndexResMsg.hpp"
#include <ctime>
#include <vector>

namespace TPCC{
struct ServerMemoryKeys;	// forward declaration

class TPCCDB {

private:
	std::ostream &os_;
	size_t itemCnt_;
	size_t customerCnt_;
	size_t warehouseCnt_;
	TPCC::RealRandomGenerator random_;
	time_t now_;
	int mrFlags_;
	std::vector<uint16_t> warehouseIDs_;

	void populate();
	void buildIndices();
	void populateWarehouse(size_t warehouseOffset);

public:
	TPCC::ItemTable 		itemTable;
	TPCC::CustomerTable 	customerTable;
	TPCC::StockTable		stockTable;
	TPCC::WarehouseTable	warehouseTable;
	TPCC::DistrictTable		districtTable;
	TPCC::OrderTable		orderTable;
	TPCC::OrderLineTable	orderLineTable;
	TPCC::NewOrderTable		newOrderTable;
	TPCC::HistoryTable		historyTable;

	TPCCDB(std::ostream &os, std::vector<uint16_t> &warehouseIDs, size_t warehouseCnt, size_t districtCnt, size_t customerCnt, size_t orderCnt, size_t orderLineCnt, size_t newOrderCnt, size_t stockCnt, size_t itemCnt, size_t hisotryCnt, size_t versionNum, TPCC::RealRandomGenerator& random, RDMAContext &context);
	void getMemoryKeys(ServerMemoryKeys *k);

	void handleLargestOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::LargestOrderForCustomerIndexRespMsg &res);
	void handleRegisterOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::IndexResponseMessage &res);
	void handleCustomerNameIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::CustomerNameIndexRespMsg &res);
	void handleLast20OrdersIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::Last20OrdersIndexResMsg &res);
	void handleOldestUndeliveredOrderIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::OldestUndeliveredOrderIndexResMsg &res);
	void handleRegisterDeliveryIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::IndexResponseMessage &res);


	void handleIndexRequest(const TPCC::IndexRequestMessage &req, TPCC::IndexResponseMessage &res);
	TPCCDB& operator=(const TPCCDB&) = delete;	// Disallow copying
	TPCCDB(const TPCCDB&) = delete;				// Disallow copying
	~TPCCDB();
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPCCDB_HPP_ */
