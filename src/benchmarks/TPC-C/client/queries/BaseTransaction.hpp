/*
 * BaseTransaction.hpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_BASETRANSACTION_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_BASETRANSACTION_HPP_

#include "../TransactionResult.hpp"
#include "../ServerContext.hpp"
#include "../DBExecutor.hpp"
#include "../SessionState.hpp"
#include "../../../../oracle/OracleContext.hpp"
#include "../../../../basic-types/PrimitiveTypes.hpp"
#include <vector>


namespace TPCC {

class BaseTransaction {
protected:
	DBExecutor executor_;

	primitive::client_id_t clientID_;
	size_t	clientCnt_;
	std::vector<ServerContext*> dsCtx_;
	SessionState *sessionState_;
	TPCC::RealRandomGenerator *random_;
	RDMAContext *context_;
	OracleContext *oracleContext_;
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;
	uint64_t nextOrderID_;
	uint64_t nextNewOrderID_;
	uint64_t nextOrderLineID_;
	uint64_t nextHistoryID_;

	virtual TransactionResult doOne() = 0;
	ServerContext* getServerContext(uint16_t wID);
	bool isRecordAccessible(Timestamp &ts);
	primitive::timestamp_t getNewCommitTimestamp();
	std::string pointer_to_string(Timestamp* ts) const;


public:
	BaseTransaction(primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector);
	virtual ~BaseTransaction();
};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_QUERIES_BASETRANSACTION_HPP_ */
