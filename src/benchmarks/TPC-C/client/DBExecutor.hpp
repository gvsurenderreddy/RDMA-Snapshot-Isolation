/*
 * DBExecutor.hpp
 *
 *  Created on: Mar 13, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_DBEXECUTOR_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_DBEXECUTOR_HPP_

#include "../tables/CustomerTable.hpp"
#include "../tables/DistrictTable.hpp"
#include "../tables/HistoryTable.hpp"
#include "../tables/ItemTable.hpp"
#include "../tables/NewOrderTable.hpp"
#include "../tables/OrderLineTable.hpp"
#include "../tables/OrderTable.hpp"
#include "../tables/StockTable.hpp"
#include "../tables/WarehouseTable.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../../../rdma-region/MemoryHandler.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../basic-types/timestamp.hpp"


namespace TPCC {

class DBExecutor {
private:
	template <typename T>
	bool isAddressInRange(uintptr_t lookupAddress, MemoryHandler<T> remoteMH);
	uint16_t getWarehouseOffsetOnServer(uint16_t wID);

public:
	virtual ~DBExecutor();
	DBExecutor();

	void getReadTimestamp(RDMARegion<primitive::timestamp_t> &, MemoryHandler<primitive::timestamp_t> &, ibv_qp *);
	void submitResult(primitive::client_id_t, RDMARegion<primitive::timestamp_t> &,  MemoryHandler<primitive::timestamp_t> &, ibv_qp *);
	void retrieveWarehouseTax(uint16_t wID, RDMARegion<TPCC::WarehouseVersion> &, MemoryHandler<TPCC::WarehouseVersion> &, ibv_qp *);
	void retrieveDistrictTax(uint16_t wID, uint8_t dID, RDMARegion<TPCC::DistrictVersion> &, MemoryHandler<TPCC::DistrictVersion> &, ibv_qp*);
	void getCustomerInformation(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<TPCC::CustomerVersion> &, MemoryHandler<TPCC::CustomerVersion> &, ibv_qp *);
	void retrieveItem(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<TPCC::ItemVersion> &, MemoryHandler<TPCC::ItemVersion> &, ibv_qp *);
	void retrieveStock(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &, MemoryHandler<TPCC::StockVersion> &, ibv_qp *);
	void retrieveStockPointerList(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<Timestamp> &, MemoryHandler<Timestamp> &, ibv_qp *, bool signaled);
	void lockStock(uint8_t olNumber, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS, RDMARegion<uint64_t> &, MemoryHandler<TPCC::StockVersion> &, ibv_qp *);
	void revertStockLock(uint8_t olNumber, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, MemoryHandler<TPCC::StockVersion> &remoteMH, ibv_qp *qp);
	void updateStockPointers(uint8_t olNumber, StockVersion *oldHead, uint16_t wID, RDMARegion<Timestamp> &, MemoryHandler<Timestamp> &, ibv_qp *);
	void updateStockOlderVersions(uint8_t olNumber, StockVersion *oldHead, uint16_t wID, RDMARegion<TPCC::StockVersion> &, MemoryHandler<TPCC::StockVersion> &, ibv_qp *);
	void retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID, RDMARegion<TPCC::DistrictVersion> &, MemoryHandler<TPCC::DistrictVersion> &, ibv_qp *);
	void insertIntoOrder(primitive::client_id_t clientID, uint64_t nextOrderID, uint16_t wID, RDMARegion<TPCC::OrderVersion> &, MemoryHandler<TPCC::OrderVersion> &, ibv_qp *);
	void insertIntoNewOrder(primitive::client_id_t clientID, uint64_t nextNewOrderID, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &, MemoryHandler<TPCC::NewOrderVersion> &, ibv_qp *);
	void updateStock(uint8_t olNumber, TPCC::StockVersion *stockV, uint16_t wID, RDMARegion<TPCC::StockVersion> &, MemoryHandler<TPCC::StockVersion> &, ibv_qp *);
	void insertIntoOrderLine(primitive::client_id_t clientID, uint64_t olID, uint8_t olNumber, uint16_t wID,  RDMARegion<TPCC::OrderLineVersion> &, MemoryHandler<TPCC::OrderLineVersion> &, ibv_qp *, bool signaled);
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_DBEXECUTOR_HPP_ */
