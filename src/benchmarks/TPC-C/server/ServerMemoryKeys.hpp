/*
 * MemoryKeys.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_SERVER_SERVERMEMORYKEYS_HPP_
#define SRC_BENCHMARKS_TPC_C_SERVER_SERVERMEMORYKEYS_HPP_

#include "../../../rdma-region/MemoryHandler.hpp"
#include "../../../basic-types/timestamp.hpp"
#include "../tables/CustomerTable.hpp"
#include "../tables/DistrictTable.hpp"
#include "../tables/HistoryTable.hpp"
#include "../tables/ItemTable.hpp"
#include "../tables/NewOrderTable.hpp"
#include "../tables/OrderLineTable.hpp"
#include "../tables/OrderTable.hpp"
#include "../tables/StockTable.hpp"
#include "../tables/WarehouseTable.hpp"
#include <infiniband/verbs.h>

namespace TPCC{
struct ServerMemoryKeys{
public:
	MemoryHandler<TPCC::CustomerVersion>	customerTableHeadVersions;
	MemoryHandler<Timestamp>				customerTabletsList;
	MemoryHandler<TPCC::CustomerVersion>	customerTableOlderVersions;

	MemoryHandler<TPCC::DistrictVersion>	districtTableHeadVersions;
	MemoryHandler<Timestamp>				districtTabletsList;
	MemoryHandler<TPCC::DistrictVersion>	districtTableOlderVersions;

	MemoryHandler<TPCC::HistoryVersion>	historyTableHeadVersions;
	MemoryHandler<Timestamp>			historyTabletsList;
	MemoryHandler<TPCC::HistoryVersion>	historyTableOlderVersions;

	MemoryHandler<TPCC::ItemVersion>	itemTableHeadVersions;
	MemoryHandler<Timestamp>			itemTabletsList;
	MemoryHandler<TPCC::ItemVersion>	itemTableOlderVersions;

	MemoryHandler<TPCC::NewOrderVersion>	newOrderTableHeadVersions;
	MemoryHandler<Timestamp>				newOrderTabletsList;
	MemoryHandler<TPCC::NewOrderVersion>	newOrderTableOlderVersions;

	MemoryHandler<TPCC::OrderLineVersion>	orderLineTableHeadVersions;
	MemoryHandler<Timestamp>				orderLineTabletsList;
	MemoryHandler<TPCC::OrderLineVersion>	orderLineTableOlderVersions;

	MemoryHandler<TPCC::OrderVersion>	orderTableHeadVersions;
	MemoryHandler<Timestamp>			orderTabletsList;
	MemoryHandler<TPCC::OrderVersion>	orderTableOlderVersions;

	MemoryHandler<TPCC::StockVersion>	stockTableHeadVersions;
	MemoryHandler<Timestamp>			stockTabletsList;
	MemoryHandler<TPCC::StockVersion>	stockTableOlderVersions;

	MemoryHandler<TPCC::WarehouseVersion>	warehouseTableHeadVersions;
	MemoryHandler<Timestamp>				warehouseTabletsList;
	MemoryHandler<TPCC::WarehouseVersion>	warehouseTableOlderVersions;



	unsigned serverInstanceNum;
};
}	// namespace TPCC



#endif /* SRC_BENCHMARKS_TPC_C_SERVER_SERVERMEMORYKEYS_HPP_ */
