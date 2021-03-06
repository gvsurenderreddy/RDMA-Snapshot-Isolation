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
	// Size: 8 Bytes

	enum IndexType {
		CUSTOMER_LAST_NAME_INDEX,
		LARGEST_ORDER_FOR_CUSTOMER_INDEX,
		REGISTER_ORDER,
		ITEMS_FOR_LAST_20_ORDERS,
		OLDEST_UNDELIVERED_ORDER,
		REGISTER_DELIVERY
	} indexType;

	bool isSuccessful;
};
}	// namespace TPCC


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_ */
