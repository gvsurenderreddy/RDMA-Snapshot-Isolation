/*
 * PaymentLocalMemory.hpp
 *
 *  Created on: Mar 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENT_PAYMENTLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENT_PAYMENTLOCALMEMORY_HPP_

#include "../../../../../rdma-region/RDMARegion.hpp"
#include "../../../tables/CustomerTable.hpp"
#include "../../../tables/DistrictTable.hpp"
#include "../../../tables/HistoryTable.hpp"
#include "../../../tables/ItemTable.hpp"
#include "../../../tables/NewOrderTable.hpp"
#include "../../../tables/OrderLineTable.hpp"
#include "../../../tables/OrderTable.hpp"
#include "../../../tables/StockTable.hpp"
#include "../../../tables/WarehouseTable.hpp"
#include "../../../../../basic-types/timestamp.hpp"

namespace TPCC {

class PaymentLocalMemory {
private:
	// RDMA region for local buffers
	RDMARegion<TPCC::CustomerVersion>	*customerHead_;
	RDMARegion<Timestamp> 				*customerTS_;
	RDMARegion<TPCC::CustomerVersion>	*customerOlderVersions_;

	RDMARegion<TPCC::DistrictVersion>	*districtHead_;
	RDMARegion<Timestamp> 				*districtTS_;
	RDMARegion<TPCC::DistrictVersion>	*districtOlderVersions_;

	RDMARegion<TPCC::HistoryVersion>	*historyHead_;
	RDMARegion<Timestamp> 				*historyTS_;
	RDMARegion<TPCC::HistoryVersion>	*historyOlderVersions_;

	RDMARegion<TPCC::WarehouseVersion>	*warehouseHead_;
	RDMARegion<Timestamp> 				*warehouseTS_;
	RDMARegion<TPCC::WarehouseVersion>	*warehouseOlderVersions_;

	RDMARegion<uint64_t>	*warehouseLockRegion_;
	RDMARegion<uint64_t>	*districtLockRegion_;
	RDMARegion<uint64_t>	*customerLockRegion_;

public:
	PaymentLocalMemory(RDMAContext &context);
	~PaymentLocalMemory();
	RDMARegion<TPCC::CustomerVersion>* getCustomerHead();
	RDMARegion<Timestamp>* getCustomerTS();
	RDMARegion<TPCC::CustomerVersion>* getCustomerOlderVersions();

	RDMARegion<TPCC::DistrictVersion>* getDistrictHead();
	RDMARegion<Timestamp>* getDistrictTS();
	RDMARegion<TPCC::DistrictVersion>* getDistrictOlderVersions();

	RDMARegion<TPCC::HistoryVersion>* getHistoryHead();
	RDMARegion<Timestamp>* getHistoryTS();
	RDMARegion<TPCC::HistoryVersion>* getHistoryOlderVersions();

	RDMARegion<TPCC::WarehouseVersion>* getWarehouseHead();
	RDMARegion<Timestamp>* getWarehouseTS();
	RDMARegion<TPCC::WarehouseVersion>* getWarehouseOlderVersions();

	RDMARegion<uint64_t>* getWarehouseLockRegion();
	RDMARegion<uint64_t>* getDistrictLockRegion();
	RDMARegion<uint64_t>* getCustomerLockRegion();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENT_PAYMENTLOCALMEMORY_HPP_ */
