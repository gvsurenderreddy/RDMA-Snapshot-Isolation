/*
 * OldestUndeliveredOrderIndexResMsg.hpp
 *
 *  Created on: Apr 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_OLDESTUNDELIVEREDORDERINDEXRESMSG_HPP_
#define SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_OLDESTUNDELIVEREDORDERINDEXRESMSG_HPP_

#include "IndexResponseMessage.hpp"
#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../tables/TPCCUtil.hpp"

namespace TPCC{
struct OldestUndeliveredOrderIndexResMsg: public IndexResponseMessage{
	// Size: 56 Bytes

	bool existUndeliveredOrder;
	uint32_t oID;
	primitive::client_id_t clientWhoOrdered;
	size_t orderRegionOffset;
	size_t newOrderRegionOffset;
	size_t orderLineRegionOffset;
	uint8_t numOfOrderlines;
};
}	// namespace TPCC




#endif /* SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_OLDESTUNDELIVEREDORDERINDEXRESMSG_HPP_ */
