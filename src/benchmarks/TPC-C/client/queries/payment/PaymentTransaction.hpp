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
#include "../../../../../recovery/RecoveryClient.hpp"

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
//
//		Network Messages Size: (V = # of versions)
//			- read: (950 + 24*V + 4 * #clients)B = (4 * #clients + 110 + 8*V + 120 + 8*V + 690 + 8*V + 60% * 12 + 8 + 8 + 8)
//			- write: (1990 + 24*V) B = (60% * 64 + 16 + 16 + 16 + 110 + 8*V + 120 + 8*V + 690 + 8*V + 110 + 120 + 690 + 60 + 4)
//			- total (for V=3 and 100 clients): 3500 B
// ************************************************
class PaymentTransaction : public BaseTransaction {
private:
	PaymentLocalMemory* localMemory_;
	PaymentCart buildCart();

public:
	PaymentTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector, RecoveryClient &recoveryClient);
	virtual ~PaymentTransaction();
	PaymentTransaction& operator=(const PaymentTransaction&) = delete;	// Disallow copying
	PaymentTransaction(const PaymentTransaction&) = delete;				// Disallow copying

	TPCC::TransactionResult doOne();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_PAYMENTTRANSACTION_HPP_ */
