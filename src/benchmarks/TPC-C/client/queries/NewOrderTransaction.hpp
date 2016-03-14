/*
 * NewOrderTransaction.hpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_

#include "BaseTransaction.hpp"
#include "NewOrderLocalMemory.hpp"
#include "../../random/randomgenerator.hpp"
#include "../../../../rdma-region/RDMAContext.hpp"

namespace TPCC {

class TPCCClient;	// it's a friend class

struct NewOrderItem {
	uint32_t I_ID;
	uint16_t OL_SUPPLY_W_ID;
	uint8_t OL_QUANTITY;
};

struct Cart{
	uint16_t wID;
	uint8_t dID;
	uint32_t cID;
	std::vector<NewOrderItem> items;

	friend std::ostream& operator<<(std::ostream& os, const Cart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | cID:" << (int)c.cID << ". --- Items (iID, suppWID):";
		for(auto const& value: c.items)
			os << " {i: " << value.I_ID << " | W: " << value.OL_SUPPLY_W_ID << "}, ";
		return os;
	}
};

class NewOrderTransaction : public BaseTransaction {
private:
	NewOrderLocalMemory* localMemory_;
	TPCC::Cart buildCart();

public:
	NewOrderTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~NewOrderTransaction();
	TPCC::TransactionResult doOne();

	NewOrderTransaction& operator=(const NewOrderTransaction&) = delete;	// Disallow copying
	NewOrderTransaction(const NewOrderTransaction&) = delete;				// Disallow copying
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_NEWORDERTRANSACTION_HPP_ */
