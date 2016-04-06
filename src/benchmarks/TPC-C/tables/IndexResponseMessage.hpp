/*
 * IndexResponseMessage.hpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_

#include  <cstring>

namespace TPCC {
struct IndexResponseMessage{
	enum IndexType {
		CUSTOMER_LAST_NAME_INDEX,
		LARGEST_ORDER_FOR_CUSTOMER_INDEX,
		REGISTER_ORDER
	} indexType;

	bool isSuccessful;

	union Result {
		struct LastNameIndex {
			uint32_t cID;
		} lastNameIndex;
		struct LargestOrderIndex {
			uint32_t oID;
			primitive::client_id_t clientWhoOrdered;
			size_t orderRegionOffset;
			size_t orderLineRegionOffset;
			uint8_t numOfOrderlines;
		} largestOrderIndex;
	} result;
};
}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_ */
