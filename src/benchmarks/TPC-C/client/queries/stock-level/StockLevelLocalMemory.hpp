/*
 * StockLevelLocalMemory.hpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELLOCALMEMORY_HPP_

#include "../../../../../rdma-region/RDMARegion.hpp"
#include "../../../tables/DistrictTable.hpp"
//#include "../../../tables/OrderLineTable.hpp"
//#include "../../../tables/OrderTable.hpp"
#include "../../../tables/StockTable.hpp"
#include "../../../../../basic-types/timestamp.hpp"

namespace TPCC {

class StockLevelLocalMemory {
private:
	std::ostream &os_;

	// RDMA region for local buffers
	RDMARegion<TPCC::DistrictVersion>	*districtHead_;
	RDMARegion<Timestamp> 				*districtTS_;
	RDMARegion<TPCC::DistrictVersion>	*districtOlderVersions_;

//	RDMARegion<TPCC::OrderVersion>	*orderHead_;
//	RDMARegion<Timestamp> 			*orderTS_;
//	RDMARegion<TPCC::OrderVersion>	*orderOlderVersions_;
//
//	RDMARegion<TPCC::OrderLineVersion>	*orderLineHead_;
//	RDMARegion<Timestamp> 			*orderLineTS_;
//	RDMARegion<TPCC::OrderLineVersion>	*orderLineOlderVersions_;

	RDMARegion<TPCC::StockVersion>	*stockHead_;
	RDMARegion<Timestamp> 			*stockTS_;
	RDMARegion<TPCC::StockVersion>	*stockOlderVersions_;

public:
	StockLevelLocalMemory(std::ostream &os, RDMAContext &context);
	~StockLevelLocalMemory();

	RDMARegion<TPCC::DistrictVersion>* getDistrictHead();
	RDMARegion<Timestamp>* getDistrictTS();
	RDMARegion<TPCC::DistrictVersion>* getDistrictOlderVersions();

//	RDMARegion<TPCC::OrderVersion>* getOrderHead();
//	RDMARegion<Timestamp>* getOrderTS();
//	RDMARegion<TPCC::OrderVersion>* getOrderOlderVersions();
//
//	RDMARegion<TPCC::OrderLineVersion>* getOrderLineHead();
//	RDMARegion<Timestamp>* getOrderLineTS();
//	RDMARegion<TPCC::OrderLineVersion>* getOrderLineOlderVersions();

	RDMARegion<TPCC::StockVersion>* getStockHead();
	RDMARegion<Timestamp>* getStockTS();
	RDMARegion<TPCC::StockVersion>* getStockOlderVersions();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELLOCALMEMORY_HPP_ */
