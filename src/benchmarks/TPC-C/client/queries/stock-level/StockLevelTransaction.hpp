/*
 * StockLevelTransaction.hpp
 *
 *  Created on: Apr 11, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELTRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELTRANSACTION_HPP_

#include "../BaseTransaction.hpp"
#include "../../../random/randomgenerator.hpp"
#include "../../../../../rdma-region/RDMAContext.hpp"
#include "StockLevelLocalMemory.hpp"


namespace TPCC {

struct StockLevelCart{
	uint16_t wID;
	uint8_t dID;
	unsigned threshold;

	friend std::ostream& operator<<(std::ostream& os, const StockLevelCart& c) {
		os << "wID:" << (int)c.wID << " | dID:" << (int)c.dID << " | Threshold: " << c.threshold;
		return os;
	}
};

// ************************************************
//	Transaction Info:
//		- Read Set: {1 district, 20 * [5-15] stocks}
//		- Indices:
//			- {OrderLine.w_id, OrderLine.d_id, OrderLine.o_id}	--> all the orderlines for the last 20 orders
// ************************************************
class StockLevelTransaction: public BaseTransaction {
private:
	StockLevelLocalMemory* localMemory_;
	StockLevelCart buildCart();
public:
	StockLevelTransaction(std::ostream &os, DBExecutor &executor, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~StockLevelTransaction();
	StockLevelTransaction& operator=(const StockLevelTransaction&) = delete;	// Disallow copying
	StockLevelTransaction(const StockLevelTransaction&) = delete;				// Disallow copying

	TPCC::TransactionResult doOne();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_STOCK_LEVEL_STOCKLEVELTRANSACTION_HPP_ */