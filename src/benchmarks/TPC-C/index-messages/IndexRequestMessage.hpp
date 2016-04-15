/*
 * IndexRequestMessage.hpp
 *
 *  Created on: Mar 31, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_

#include "../../../basic-types/PrimitiveTypes.hpp"
#include  <cstring>

namespace TPCC {
struct IndexRequestMessage {
	primitive::client_id_t clientID;

	enum OperationType {
		UPDATE,
		LOOKUP,
		DELETE,
		TERMINATE
	} operationType;

	enum IndexType {
		CUSTOMER_LAST_NAME_INDEX,
		LARGEST_ORDER_FOR_CUSTOMER_INDEX,
		REGISTER_ORDER,
		ITEMS_FOR_LAST_20_ORDERS,
		OLDEST_UNDELIVERED_ORDER
	} indexType;

	union Parameters {
		struct LastNameIndex {
			uint16_t warehouseOffset;
			uint8_t dID;
			char customerLastName[17];
		} lastNameIndex;

		struct LargestOrderIndex {
			uint16_t warehouseOffset;
			uint8_t dID;
			uint32_t cID;
		} largestOrderIndex;

		struct RegisterOrderIndex {
			uint16_t warehouseOffset;
			uint8_t dID;
			uint32_t cID;
			uint32_t oID;
			size_t orderRegionOffset;
			size_t newOrderRegionOffset;
			size_t orderLineRegionOffset;
			uint8_t numOfOrderlines;
		} registerOrderIndex;

		struct Last20OrdersIndex {
			uint16_t warehouseOffset;
			uint8_t dID;
			uint32_t D_NEXT_O_ID;
		} last20OrdersIndex;

		struct OldestUndeliveredOrderIndex{
			uint16_t warehouseOffset;
			uint8_t dID;
		} oldestUndeliveredOrderIndex;
	} parameters;
};

}	// namespace TPCC




#endif /* SRC_BENCHMARKS_TPC_C_TABLES_INDEXREQUESTMESSAGE_HPP_ */
