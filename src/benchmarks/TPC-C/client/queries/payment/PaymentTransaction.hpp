/*
 * PaymentTransaction.hpp
 *
 *  Created on: Mar 15, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENTTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENTTRANSACTION_HPP_

#include "../BaseTransaction.hpp"
#include "PaymentLocalMemory.hpp"
#include "../../../random/randomgenerator.hpp"
#include "../../../../../rdma-region/RDMAContext.hpp"

namespace TPCC {

struct PaymentCart{
	uint16_t wID;
	uint8_t dID;
	uint16_t residentWarehouseID;
	enum {
		LAST_NAME,
		ID
	} customerSelectionMode;
	char cLastName[17];
	uint32_t cID;
	float hAmount;

	friend std::ostream& operator<<(std::ostream& os, const PaymentCart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | resident_W_ID:" << (int)c.residentWarehouseID;
		c.customerSelectionMode == LAST_NAME ? (os << " | LAST_NAME: " << c.cLastName) :  (os << " | cID: " << c.cID);
		os << " | hAmount: " << c.hAmount;
		return os;
	}
};

// ************************************************
//	Transaction Info:
//		- Read Set: {1 warehouse, 1 district, 1 customer}
//		- Write Set:
//			- Modify set: {1 warehouse, 1 district, 1 customer}
//			- New: {1 history}
// ************************************************
class PaymentTransaction : public BaseTransaction {
private:
	PaymentLocalMemory* localMemory_;
	PaymentCart buildCart();

public:
	PaymentTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~PaymentTransaction();
	PaymentTransaction& operator=(const PaymentTransaction&) = delete;	// Disallow copying
	PaymentTransaction(const PaymentTransaction&) = delete;				// Disallow copying

	TPCC::TransactionResult doOne();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENTTRANSACTION_HPP_ */
