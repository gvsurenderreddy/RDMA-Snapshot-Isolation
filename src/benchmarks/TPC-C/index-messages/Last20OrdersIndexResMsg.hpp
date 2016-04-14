/*
 * Last20OrdersIndexResMsg.hpp
 *
 *  Created on: Apr 12, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LAST20ORDERSINDEXRESMSG_HPP_
#define SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LAST20ORDERSINDEXRESMSG_HPP_

#include "IndexResponseMessage.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../tables/TPCCUtil.hpp"

namespace TPCC{
struct Last20OrdersIndexResMsg: public IndexResponseMessage{
	size_t orderlinesCnt;
	uint32_t itemIDs[20 * tpcc_settings::ORDER_MAX_OL_CNT];
};
}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LAST20ORDERSINDEXRESMSG_HPP_ */
