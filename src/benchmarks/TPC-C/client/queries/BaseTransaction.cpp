/*
 * BaseTransaction.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#include "BaseTransaction.hpp"
#include <sstream>	// for std::ostringstream

namespace TPCC {

BaseTransaction::BaseTransaction(std::string transactionName, primitive::client_id_t clientID, size_t clientCnt, std::vector<ServerContext*> dsCtx, SessionState *sessionState, RealRandomGenerator *random, RDMAContext *context, OracleContext *oracleContext, RDMARegion<primitive::timestamp_t> *localTimestampVector)
: transactionName_(transactionName),
  clientID_(clientID),
  clientCnt_(clientCnt),
  dsCtx_(dsCtx),
  sessionState_(sessionState),
  random_(random),
  context_(context),
  oracleContext_(oracleContext),
  localTimestampVector_(localTimestampVector),
  nextOrderID_(0),
  nextNewOrderID_(0),
  nextOrderLineID_(0),
  nextHistoryID_(0){
}

BaseTransaction::~BaseTransaction() {
	// TODO Auto-generated destructor stub
}

std::string BaseTransaction::getTransactionName() const{
	return transactionName_;
}


bool BaseTransaction::isRecordAccessible(Timestamp &ts){
	if (ts.isLocked())
		// item is already locked
		return false;

	primitive::client_id_t committingClient = ts.getClientID();
	if (committingClient == clientID_)
		// regardless of whether the version matches the snapshot or not, the version is installed by the client itself, so is valid
		// when is it useful? when using adaptive abort rate control.
		return true;
	if (ts.getTimestamp() > localTimestampVector_->getRegion()[committingClient])
		// from a later snapshot, so not useful
		return false;

	return true;
}

TPCC::ServerContext* BaseTransaction::getServerContext(uint16_t wID){
	size_t serverNum = (int) (wID / config::tpcc_settings::WAREHOUSE_PER_SERVER);
	return dsCtx_[serverNum];
}

primitive::timestamp_t BaseTransaction::getNewCommitTimestamp() {
	primitive::timestamp_t output;
	output = (primitive::timestamp_t)(sessionState_->getNextEpoch() * clientCnt_ + clientID_);
	sessionState_->advanceEpoch();
	return output;
}

std::string BaseTransaction::pointer_to_string(Timestamp* ts) const{
	std::ostringstream stream;
	for (size_t i = 0; i < config::tpcc_settings::VERSION_NUM; i++)
		stream << ts[i] << ", ";
	return stream.str();
}

} /* namespace TPCC */
