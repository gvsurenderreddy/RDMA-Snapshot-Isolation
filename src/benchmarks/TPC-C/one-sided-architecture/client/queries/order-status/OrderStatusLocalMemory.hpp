/*
 * OrderStatusLocalMemory.hpp
 *
 *  Created on: Apr 4, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSLOCALMEMORY_HPP_

#include "../../../../../../rdma-region/RDMARegion.hpp"
#include "../../../../tables/CustomerTable.hpp"
#include "../../../../tables/OrderLineTable.hpp"
#include "../../../../tables/OrderTable.hpp"
#include "../../../../../../basic-types/timestamp.hpp"

namespace TPCC {

class OrderStatusLocalMemory {
private:
	std::ostream &os_;

	// RDMA region for local buffers
	RDMARegion<TPCC::OrderVersion>	*orderHead_;
	RDMARegion<Timestamp> 			*orderTS_;
	RDMARegion<TPCC::OrderVersion>	*orderOlderVersions_;

	RDMARegion<TPCC::OrderLineVersion>	*orderLineHead_;
	RDMARegion<Timestamp> 			*orderLineTS_;
	RDMARegion<TPCC::OrderLineVersion>	*orderLineOlderVersions_;

public:
	OrderStatusLocalMemory(std::ostream &os, RDMAContext &context);
	~OrderStatusLocalMemory();

	RDMARegion<TPCC::OrderVersion>* getOrderHead();
	RDMARegion<Timestamp>* getOrderTS();
	RDMARegion<TPCC::OrderVersion>* getOrderOlderVersions();

	RDMARegion<TPCC::OrderLineVersion>* getOrderLineHead();
	RDMARegion<Timestamp>* getOrderLineTS();
	RDMARegion<TPCC::OrderLineVersion>* getOrderLineOlderVersions();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSLOCALMEMORY_HPP_ */
