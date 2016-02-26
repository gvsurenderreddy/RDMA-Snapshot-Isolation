/*
 * TPCCDB.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPCCDB_HPP_
#define SRC_BENCHMARKS_TPCCDB_HPP_

#include "tables/CustomerTable.hpp"
#include "tables/ItemTable.hpp"
#include "tables/OrderTable.hpp"
#include "tables/WarehouseTable.hpp"
#include "tables/OrderLineTable.hpp"
#include "tables/NewOrderTable.hpp"
#include "tables/StockTable.hpp"
#include "tables/DistrictTable.hpp"
#include "../../rdma-region/RDMAContext.hpp"
#include "server/ServerMemoryKeys.hpp"
#include "random/randomgenerator.hpp"


#include <ctime>

class TPCCDB {
private:
	size_t itemCnt_;
	size_t customerCnt_;
	size_t warehouseCnt_;
	TPCC::RealRandomGenerator random_;
	time_t now_;
	int mrFlags_;

public:
	TPCC::ItemTable 		itemTable;
	TPCC::CustomerTable 	customerTable;
	TPCC::StockTable		stockTable;
	TPCC::WarehouseTable	warehouseTable;
	TPCC::DistrictTable		districtTable;
	TPCC::OrderTable		orderTable;
	TPCC::OrderLineTable	orderLineTable;
	TPCC::NewOrderTable		newOrderTable;

	TPCCDB(size_t warehouseCnt, size_t districtCnt, size_t customerCnt, size_t orderCnt, size_t orderLineCnt, size_t newOrderCnt, size_t stockCnt, size_t itemCnt, size_t versionNum, TPCC::RealRandomGenerator& random, RDMAContext &context);
	void populate();
	void getMemoryKeys(ServerMemoryKeys *k);
	TPCCDB& operator=(const TPCCDB&) = delete;	// Disallow copying
	TPCCDB(const TPCCDB&) = delete;				// Disallow copying
	~TPCCDB();
};

#endif /* SRC_BENCHMARKS_TPCCDB_HPP_ */
