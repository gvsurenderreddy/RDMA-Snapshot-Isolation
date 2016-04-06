/*
 * OrderStatusTransaction.hpp
 *
 *  Created on: Apr 4, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSTRANSACTION_HPP_

#include "../BaseTransaction.hpp"
#include "OrderStatusLocalMemory.hpp"
#include "../../../random/randomgenerator.hpp"
#include "../../../../../rdma-region/RDMAContext.hpp"


namespace TPCC {

struct OrderStatusCart{
	uint16_t wID;
	uint8_t dID;
	enum {
		LAST_NAME,
		ID
	} customerSelectionMode;
	char cLastName[17];
	uint32_t cID;

	friend std::ostream& operator<<(std::ostream& os, const OrderStatusCart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID;
		c.customerSelectionMode == LAST_NAME ? (os << " | LAST_NAME: " << c.cLastName) :  (os << " | cID: " << c.cID);
		return os;
	}
};

// ************************************************
//	Transaction Info:
//		- Read Set: {1 order, 5-15 orderlines}
//		- Indices:
//			- Customer.last_name					--> Customer.c_id
//			- {Order.w_id, Order.d_id, Order.c_id}	--> largest Order.o_id
// ************************************************
class OrderStatusTransaction: public BaseTransaction {
private:
	OrderStatusLocalMemory* localMemory_;
	OrderStatusCart buildCart();
public:
	OrderStatusTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~OrderStatusTransaction();
	OrderStatusTransaction& operator=(const OrderStatusTransaction&) = delete;	// Disallow copying
	OrderStatusTransaction(const OrderStatusTransaction&) = delete;				// Disallow copying

	TPCC::TransactionResult doOne();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_ORDER_STATUS_ORDERSTATUSTRANSACTION_HPP_ */
