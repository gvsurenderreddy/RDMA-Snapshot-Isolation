/*
 * LargestOrderForCustomerIndexRespMsg.hpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LARGESTORDERFORCUSTOMERINDEXRESPMSG_HPP_
#define SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LARGESTORDERFORCUSTOMERINDEXRESPMSG_HPP_

#include "IndexResponseMessage.hpp"

namespace TPCC{

struct LargestOrderForCustomerIndexRespMsg: public IndexResponseMessage{
	// Size: 40 bytes

	uint32_t oID;
	primitive::client_id_t clientWhoOrdered;
	size_t orderRegionOffset;
	size_t orderLineRegionOffset;
	uint8_t numOfOrderlines;
};
}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_LARGESTORDERFORCUSTOMERINDEXRESPMSG_HPP_ */
