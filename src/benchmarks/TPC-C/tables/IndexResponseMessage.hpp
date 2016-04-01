/*
 * IndexResponseMessage.hpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_

namespace TPCC {
struct IndexResponseMessage{
	enum IndexType {
		CUSTOMER_LAST_NAME_INDEX
	} indexType;

	bool isSuccessful;

	union Result {
		struct LastNameIndex {
			uint32_t cID;
		} lastNameIndex;
	} result;
};
}	// namespace TPCC



#endif /* SRC_BENCHMARKS_TPC_C_TABLES_INDEXRESPONSEMESSAGE_HPP_ */
