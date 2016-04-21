/*
 * DeliveryTransaction.hpp
 *
 *  Created on: Apr 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYTRANSACTION_HPP_

#include "../BaseTransaction.hpp"
#include "DeliveryLocalMemory.hpp"
#include "../../../random/randomgenerator.hpp"
#include "../../../../../rdma-region/RDMAContext.hpp"

namespace TPCC {

struct DeliveryCart{
	uint16_t wID;
	uint8_t oCarrierID;
	time_t olDeliveryD;

	friend std::ostream& operator<<(std::ostream& os, const DeliveryCart& c) {
		os << "wID:" << (int)c.wID << " | oCarrierID:" << (int)c.oCarrierID;
		return os;
	}
};

// ************************************************
//	Transaction Info:
//		- Read Set: {10 new_orders, 10 orders, 10 * [5-15] orderlines, 10 customers}
//		- Write Set:
//			- Modify set: {10 new_orders, 10 orders, 10 * [5-15] orderlines, 10 customers}
//		- Indices:
//			- {NewOrder.w_id, NewOrder.d_id}	--> smallest NewOrder.o_id, pointers to the entries in order, new order, and orderline tables
//
//		Network Messages Size: (V = number of versions)
//			- read: (16140 + 80*V + 40 * #clients)B = 10 * (56 + 40 + 20 + 800 + 690 + 8*V + 4*#clients + 8)B		--> 2 assumptions: 1-there is at least one undelivered order per district (10 full transactions). 2- without considering fetching older versions
//			- write: (25800 + 80*V)B = 10 * (64 + 16 + 16 + 160 + 16 + 8*V + 690 + 40 + 20 + 800 + 690 + 64 + 4)
// ************************************************
class DeliveryTransaction : public BaseTransaction {
private:
	DeliveryLocalMemory* localMemory_;
	DeliveryCart buildCart();

public:
	DeliveryTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~DeliveryTransaction();
	TPCC::TransactionResult doOne();

	DeliveryTransaction& operator=(const DeliveryTransaction&) = delete;	// Disallow copying
	DeliveryTransaction(const DeliveryTransaction&) = delete;				// Disallow copying
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_DELIVERY_DELIVERYTRANSACTION_HPP_ */
