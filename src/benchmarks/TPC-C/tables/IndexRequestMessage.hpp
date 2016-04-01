/*
 * IndexRequestMessage.hpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_

#include "../../../basic-types/PrimitiveTypes.hpp"

namespace TPCC {
struct IndexRequestMessage {
	primitive::client_id_t clientID;

	enum OperationType {
		INSERT,
		LOOKUP,
		DELETE,
		TERMINATE
	} operationType;

	enum IndexType {
		CUSTOMER_LAST_NAME_INDEX
	} indexType;

	union Parameters {
		struct LastNameIndex {
			uint16_t warehouseOffset;
			uint8_t dID;
			char customerLastName[17];
		} lastNameIndex;
	} parameters;
};

}	// namespace TPCC




#endif /* SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_ */
