/*
 * CustomerNameIndexRespMsg.hpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_CUSTOMERNAMEINDEXRESPMSG_HPP_
#define SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_CUSTOMERNAMEINDEXRESPMSG_HPP_

#include "IndexResponseMessage.hpp"

namespace TPCC {

struct CustomerNameIndexRespMsg : public IndexResponseMessage {
	uint32_t cID;
};
}	// namespace TPCC

#endif /* SRC_BENCHMARKS_TPC_C_INDEX_MESSAGES_CUSTOMERNAMEINDEXRESPMSG_HPP_ */
