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
#include "../index-messages/IndexRequestMessage.hpp"
#include "../index-messages/IndexResponseMessage.hpp"
#include "../index-messages/CustomerNameIndexRespMsg.hpp"
#include "../index-messages/LargestOrderForCustomerIndexRespMsg.hpp"
#include "../index-messages/Last20OrdersIndexResMsg.hpp"
#include "../index-messages/Last20OrdersIndexResMsg.hpp"
#include "../index-messages/OldestUndeliveredOrderIndexResMsg.hpp"
#include "ServerContext.hpp"

namespace TPCC {

class DBExecutor {
private:
	std::ostream &os_;
	std::vector<ServerContext*> dsCtx_;

public:
	DBExecutor(std::ostream &os, std::vector<ServerContext *> dsCtx);
	virtual ~DBExecutor();
	DBExecutor& operator=(const DBExecutor&) = delete;	// Disallow copying
	DBExecutor(const DBExecutor&) = delete;				// Disallow copying

	void lookupCustomerByLastName(primitive::client_id_t, uint16_t wID, uint8_t dID, const char *cLastName, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::CustomerNameIndexRespMsg> &responseRegion, ibv_qp *qp, bool signaled);
	void getLastOrderOfCustomer(primitive::client_id_t, uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<TPCC::IndexRequestMessage> &, RDMARegion<TPCC::IndexResponseMessage> &, ibv_qp *qp, bool signaled);
	void getLastOrderOfCustomer(primitive::client_id_t, uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg> &responseRegion, ibv_qp *qp, bool signaled);
	void registerOrder(primitive::client_id_t, uint16_t wID, uint8_t dID, uint32_t cID, uint32_t oID, size_t orderRegionOffset, size_t newOrderRegionOffset, size_t orderLineRegionOffset, uint8_t numOfOrderlines, RDMARegion<TPCC::IndexRequestMessage> &, RDMARegion<TPCC::IndexResponseMessage> &, ibv_qp *qp, bool signaled);
	void getDistinctItemsForLastTwentyOrders(primitive::client_id_t, uint16_t wID, uint8_t dID, uint32_t D_NEXT_O_ID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::Last20OrdersIndexResMsg> &responseRegion, ibv_qp *qp, bool signaled);
	void getOldestUndeliveredOrder(primitive::client_id_t, uint16_t wID, uint8_t dID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg> &responseRegion, ibv_qp *qp, bool signaled);
	void registerDelivery(primitive::client_id_t, uint16_t wID, uint8_t dID, uint32_t oID, RDMARegion<TPCC::IndexRequestMessage> &requestRegion, RDMARegion<TPCC::IndexResponseMessage> &responseRegion, ibv_qp *qp, bool signaled);

	void getReadTimestamp(RDMARegion<primitive::timestamp_t> &, MemoryHandler<primitive::timestamp_t> &, ibv_qp *, bool signaled);
	void submitResult(primitive::client_id_t, RDMARegion<primitive::timestamp_t> &,  MemoryHandler<primitive::timestamp_t> &, ibv_qp *, bool signaled);

	void retrieveWarehouse(uint16_t wID, RDMARegion<WarehouseVersion> &, bool signaled);
	void retrieveWarehouseOlderVersion(uint16_t wID, size_t versionOffset, RDMARegion<WarehouseVersion> &, bool signaled);
	void retrieveWarehousePointerList(uint16_t wID, RDMARegion<Timestamp> &, bool signaled);
	void lockWarehouse(TPCC::WarehouseVersion &warehouseV, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void revertWarehouseLock(RDMARegion<TPCC::WarehouseVersion> &, bool signaled);
	void updateWarehousePointers(TPCC::WarehouseVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled);
	void updateWarehouseOlderVersions(RDMARegion<TPCC::WarehouseVersion> &localRegion, bool signaled);
	void updateWarehouse(RDMARegion<TPCC::WarehouseVersion> &localRegion, bool signaled);

	void retrieveDistrict(uint16_t wID, uint8_t dID, RDMARegion<DistrictVersion> &, bool signaled);
	void retrieveDistrictOlderVersion(uint16_t wID, uint8_t dID, size_t versionOffset, RDMARegion<DistrictVersion> &, bool signaled);
	void retrieveAndIncrementDistrictNextOID(uint16_t wID, uint8_t dID, RDMARegion<TPCC::DistrictVersion> &, bool signaled);
	void retrieveDistrictPointerList(uint16_t wID, uint8_t dID, RDMARegion<Timestamp> &, bool signaled);
	void lockDistrict(TPCC::DistrictVersion &districtV, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void revertDistrictLock(RDMARegion<TPCC::DistrictVersion> &, bool signaled);
	void updateDistrictPointers(TPCC::DistrictVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled);
	void updateDistrictOlderVersions(RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled);
	void updateDistrict(RDMARegion<TPCC::DistrictVersion> &localRegion, bool signaled);

	void retrieveCustomer(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<TPCC::CustomerVersion> &,  bool signaled);
	void retrieveCustomerOlderVersion(uint16_t wID, uint8_t dID, uint32_t cID, size_t versionOffset, RDMARegion<TPCC::CustomerVersion> &, bool signaled);
	void retrieveCustomerPointerList(uint16_t wID, uint8_t dID, uint32_t cID, RDMARegion<Timestamp> &, bool signaled);
	void lockCustomer(TPCC::CustomerVersion &customerV, Timestamp &newTS, RDMARegion<uint64_t> &localRegion, bool signaled);
	void revertCustomerLock(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled);
	void updateCustomerPointers(TPCC::CustomerVersion &oldHead, RDMARegion<Timestamp> &localRegion, bool signaled);
	void updateCustomerOlderVersions(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled);
	void updateCustomer(RDMARegion<TPCC::CustomerVersion> &localRegion, bool signaled);

	void retrieveItem(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<TPCC::ItemVersion> &, bool signaled);

	void lockStock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, Timestamp &oldTS, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void retrieveStock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &, bool signaled);
	void retrieveStockOlderVersion(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, size_t versionOffset, RDMARegion<StockVersion> &, bool signaled);
	void retrieveStockPointerList(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<Timestamp> &, bool signaled);
	void revertStockLock(size_t offsetInLocalRegion, uint32_t iID, uint16_t wID, RDMARegion<TPCC::StockVersion> &localRegion, bool signaled);
	void updateStockPointers(size_t offsetInLocalRegion, StockVersion *oldHead, uint16_t wID, RDMARegion<Timestamp> &, bool signaled);
	void updateStockOlderVersions(size_t offsetInLocalRegion, StockVersion *oldHead, uint16_t wID, RDMARegion<TPCC::StockVersion> &, bool signaled);
	void updateStock(size_t offsetInLocalRegion, TPCC::StockVersion *stockV, uint16_t wID, RDMARegion<TPCC::StockVersion> &, bool signaled);

	void retrieveOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID,  RDMARegion<TPCC::OrderVersion> &, bool signaled);
	void insertIntoOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderVersion> &, bool signaled);
	void lockOrder(TPCC::OrderVersion &orderV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void revertOrderLock(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderVersion> &, bool signaled);

	void retrieveNewOrder(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &, bool signaled);
	void insertIntoNewOrder(primitive::client_id_t clientID, uint64_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &, bool signaled);
	void lockNewOrder(TPCC::NewOrderVersion &orderV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void revertNewOrderLock(primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::NewOrderVersion> &, bool signaled);

	void insertIntoOrderLine(primitive::client_id_t clientID, uint64_t clientRegionOffset, uint8_t offsetInLocalRegion, uint16_t wID,  RDMARegion<TPCC::OrderLineVersion> &, bool signaled);
	void retrieveOrderLines(primitive::client_id_t clientID, uint16_t wID, size_t clientRegionOffset, uint8_t numOfOrderlines,  RDMARegion<TPCC::OrderLineVersion> &, bool signaled);
	void lockOrderLine(size_t offsetInLocalRegion, TPCC::OrderLineVersion &orderLineV, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, Timestamp &newTS, RDMARegion<uint64_t> &, bool signaled);
	void revertOrderLineLock(size_t offsetInLocalRegion, primitive::client_id_t clientID, size_t clientRegionOffset, uint16_t wID, RDMARegion<TPCC::OrderLineVersion> &, bool signaled);

	void insertIntoHistory(primitive::client_id_t clientID, uint64_t hID, RDMARegion<TPCC::HistoryVersion> &localRegion, bool signaled);
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_DBEXECUTOR_HPP_ */
