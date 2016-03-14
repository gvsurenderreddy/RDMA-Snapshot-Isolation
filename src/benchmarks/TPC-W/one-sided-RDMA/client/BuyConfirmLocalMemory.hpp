/*
 * BuyConfirmLocalMemory.hpp
 *
 *  Created on: Feb 26, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_BUYCONFIRMLOCALMEMORY_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_BUYCONFIRMLOCALMEMORY_HPP_

#include "../../../../rdma-region/RDMARegion.hpp"
#include "../../tables/CustomerTable.hpp"
#include "../../tables/DistrictTable.hpp"
#include "../../tables/HistoryTable.hpp"
#include "../../tables/ItemTable.hpp"
#include "../../tables/NewOrderTable.hpp"
#include "../../tables/OrderLineTable.hpp"
#include "../../tables/OrderTable.hpp"
#include "../../tables/StockTable.hpp"
#include "../../tables/WarehouseTable.hpp"
#include "../../../../basic-types/timestamp.hpp"

namespace TPCW{
class BuyConfirmLocalMemory {
private:
	// RDMA region for local buffers
	RDMARegion<TPCW::ItemVersion>	*itemHead_;
	RDMARegion<Timestamp> 	*itemTS_;
	RDMARegion<TPCW::ItemVersion>	*itemOlderVersions_;

	RDMARegion<uint64_t>	*lockRegion_;

public:
	BuyConfirmLocalMemory(RDMAContext &context);
	~BuyConfirmLocalMemory();
	RDMARegion<TPCW::ItemVersion>* getItemHead();
	RDMARegion<Timestamp>* getItemTS();
	RDMARegion<TPCW::ItemVersion>* getItemOlderVersions();

	RDMARegion<uint64_t>* getLockRegion();
};
}	// namespace TPCW

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_BUYCONFIRMLOCALMEMORY_HPP_ */
