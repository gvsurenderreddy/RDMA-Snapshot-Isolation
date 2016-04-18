/*
 * DeliveryLocalMemory.hpp
 *
 *  Created on: Apr 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYLOCALMEMORY_HPP_

#include "../../../rdma-region/RDMARegion.hpp"
#include "../tables/CustomerTable.hpp"
#include "../tables/NewOrderTable.hpp"
#include "../tables/OrderLineTable.hpp"
#include "../tables/OrderTable.hpp"
#include "../../../basic-types/timestamp.hpp"

namespace TPCC{
class DeliveryLocalMemory {
private:
	std::ostream &os_;
	// RDMA region for local buffers
	RDMARegion<TPCC::CustomerVersion>	*customerHead_;
	RDMARegion<Timestamp> 				*customerTS_;
	RDMARegion<TPCC::CustomerVersion>	*customerOlderVersions_;

	RDMARegion<TPCC::NewOrderVersion>	*newOrderHead_;
	RDMARegion<Timestamp> 				*newOrderTS_;
	RDMARegion<TPCC::NewOrderVersion>	*newOrderOlderVersions_;

	RDMARegion<TPCC::OrderVersion>	*orderHead_;
	RDMARegion<Timestamp> 			*orderTS_;
	RDMARegion<TPCC::OrderVersion>	*orderOlderVersions_;

	RDMARegion<TPCC::OrderLineVersion>	*orderLineHead_;
	RDMARegion<Timestamp> 				*orderLineTS_;
	RDMARegion<TPCC::OrderLineVersion>	*orderLineOlderVersions_;

	RDMARegion<uint64_t>	*customerLockRegion_;
	RDMARegion<uint64_t>	*orderLockRegion_;
	RDMARegion<uint64_t>	*newOrderLockRegion_;
	RDMARegion<uint64_t>	*orderLinesLocksRegion_;

public:
	DeliveryLocalMemory(std::ostream &os, RDMAContext &context);
	~DeliveryLocalMemory();
	RDMARegion<TPCC::CustomerVersion>* getCustomerHead();
	RDMARegion<Timestamp>* getCustomerTS();
	RDMARegion<TPCC::CustomerVersion>* getCustomerOlderVersions();

	RDMARegion<TPCC::NewOrderVersion>* getNewOrderHead();
	RDMARegion<Timestamp>* getNewOrderTS();
	RDMARegion<TPCC::NewOrderVersion>* getNewOrderOlderVersions();

	RDMARegion<TPCC::OrderVersion>* getOrderHead();
	RDMARegion<Timestamp>* getOrderTS();
	RDMARegion<TPCC::OrderVersion>* getOrderOlderVersions();

	RDMARegion<TPCC::OrderLineVersion>* getOrderLineHead();
	RDMARegion<Timestamp>* getOrderLineTS();
	RDMARegion<TPCC::OrderLineVersion>* getOrderLineOlderVersions();

	RDMARegion<uint64_t>* getCustomerLockRegion();
	RDMARegion<uint64_t>* getOrderLockRegion();
	RDMARegion<uint64_t>* getNewOrderLockRegion();
	RDMARegion<uint64_t>* getOrderLinesLocksRegion();
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYLOCALMEMORY_HPP_ */
