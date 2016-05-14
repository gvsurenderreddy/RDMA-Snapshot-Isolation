/*
 * LocalMemory.hpp
 *
 *  Created on: Feb 26, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_NEWORDERLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_NEWORDERLOCALMEMORY_HPP_

#include "../../../../../../rdma-region/RDMARegion.hpp"
#include "../../../../tables/CustomerTable.hpp"
#include "../../../../tables/DistrictTable.hpp"
#include "../../../../tables/HistoryTable.hpp"
#include "../../../../tables/ItemTable.hpp"
#include "../../../../tables/NewOrderTable.hpp"
#include "../../../../tables/OrderLineTable.hpp"
#include "../../../../tables/OrderTable.hpp"
#include "../../../../tables/StockTable.hpp"
#include "../../../../tables/WarehouseTable.hpp"
#include "../../../../../../basic-types/timestamp.hpp"

namespace TPCC{
class NewOrderLocalMemory {
private:
	std::ostream &os_;
	// RDMA region for local buffers
	RDMARegion<TPCC::CustomerVersion>	*customerHead_;
	RDMARegion<Timestamp> 				*customerTS_;
	RDMARegion<TPCC::CustomerVersion>	*customerOlderVersions_;

	RDMARegion<TPCC::DistrictVersion>	*districtHead_;
	RDMARegion<Timestamp> 				*districtTS_;
	RDMARegion<TPCC::DistrictVersion>	*districtOlderVersions_;

	RDMARegion<TPCC::ItemVersion>	*itemHead_;
	RDMARegion<Timestamp> 			*itemTS_;
	RDMARegion<TPCC::ItemVersion>	*itemOlderVersions_;

	RDMARegion<TPCC::NewOrderVersion>	*newOrderHead_;
	RDMARegion<Timestamp> 				*newOrderTS_;
	RDMARegion<TPCC::NewOrderVersion>	*newOrderOlderVersions_;

	RDMARegion<TPCC::OrderLineVersion>	*orderLineHead_;
	RDMARegion<Timestamp> 				*orderLineTS_;
	RDMARegion<TPCC::OrderLineVersion>	*orderLineOlderVersions_;

	RDMARegion<TPCC::OrderVersion>	*orderHead_;
	RDMARegion<Timestamp> 			*orderTS_;
	RDMARegion<TPCC::OrderVersion>	*orderOlderVersions_;

	RDMARegion<TPCC::StockVersion>	*stockHead_;
	RDMARegion<Timestamp> 			*stockTS_;
	RDMARegion<TPCC::StockVersion>	*stockOlderVersions_;

	RDMARegion<TPCC::WarehouseVersion>	*warehouseHead_;
	RDMARegion<Timestamp> 				*warehouseTS_;
	RDMARegion<TPCC::WarehouseVersion>	*warehouseOlderVersions_;

	RDMARegion<uint64_t>	*districtLockRegion_;
	RDMARegion<uint64_t>	*stocksLocksRegion_;

public:
	NewOrderLocalMemory(std::ostream &os, RDMAContext &context);
	~NewOrderLocalMemory();
	RDMARegion<TPCC::CustomerVersion>* getCustomerHead();
	RDMARegion<Timestamp>* getCustomerTS();
	RDMARegion<TPCC::CustomerVersion>* getCustomerOlderVersions();

	RDMARegion<TPCC::DistrictVersion>* getDistrictHead();
	RDMARegion<Timestamp>* getDistrictTS();
	RDMARegion<TPCC::DistrictVersion>* getDistrictOlderVersions();

	RDMARegion<TPCC::ItemVersion>* getItemHead();
	RDMARegion<Timestamp>* getItemTS();
	RDMARegion<TPCC::ItemVersion>* getItemOlderVersions();

	RDMARegion<TPCC::NewOrderVersion>* getNewOrderHead();
	RDMARegion<Timestamp>* getNewOrderTS();
	RDMARegion<TPCC::NewOrderVersion>* getNewOrderOlderVersions();

	RDMARegion<TPCC::OrderLineVersion>* getOrderLineHead();
	RDMARegion<Timestamp>* getOrderLineTS();
	RDMARegion<TPCC::OrderLineVersion>* getOrderLineOlderVersions();

	RDMARegion<TPCC::OrderVersion>* getOrderHead();
	RDMARegion<Timestamp>* getOrderTS();
	RDMARegion<TPCC::OrderVersion>* getOrderOlderVersions();

	RDMARegion<TPCC::StockVersion>* getStockHead();
	RDMARegion<Timestamp>* getStockTS();
	RDMARegion<TPCC::StockVersion>* getStockOlderVersions();

	RDMARegion<TPCC::WarehouseVersion>* getWarehouseHead();
	RDMARegion<Timestamp>* getWarehouseTS();
	RDMARegion<TPCC::WarehouseVersion>* getWarehouseOlderVersions();

	RDMARegion<uint64_t>* getDistrictLockRegion();
	RDMARegion<uint64_t>* getStocksLocksRegion();
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_NEWORDERLOCALMEMORY_HPP_ */
