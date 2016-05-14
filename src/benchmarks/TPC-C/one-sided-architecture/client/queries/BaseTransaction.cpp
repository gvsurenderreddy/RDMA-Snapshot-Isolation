/*
 * BaseTransaction.cpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#include "BaseTransaction.hpp"
#include <sstream>	// for std::ostringstream

namespace TPCC {

BaseTransaction::BaseTransaction(std::string transactionName, TPCCClient &client, DBExecutor &executor)
:   os_(*client.os_),
	transactionName_(transactionName),
	client_(client),
	executor_(executor),
	clientID_(client.clientID_),
	clientCnt_(client.clientCnt_),
	dsCtx_(client.dsCtx_),
	sessionState_(*client.sessionState_),
	random_(client.random_),
	context_(*client.context_),
	oracleContext_(*client.oracleContext_),
	localTimestampVector_(*client.localTimestampVector_),
	recoveryClient_(*client.recoveryClient_),
	oracleReader_(client.oracleReader_),
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

bool BaseTransaction::isRecordAccessible(const Timestamp &ts) const{
	if (ts.isLocked())	// item is already locked
		return false;

	if (ts.isDeleted())	// item is deleted (or not valid)
		return false;

	primitive::client_id_t committingClient = ts.getClientID();
	if (committingClient == client_.clientID_)
		// regardless of whether the version matches the snapshot or not, the version is installed by the client itself, so is valid
		// when is it useful? when using adaptive abort rate control.
		return true;
	if (ts.getTimestamp() > client_.localTimestampVector_->getRegion()[committingClient])
		// from a later snapshot, so not useful
		return false;

	return true;
}

int BaseTransaction::findValidVersion(const Timestamp *timestampList, const size_t versionCnt) const{
	for (int i = 0; i < (int)versionCnt; i++){
		if (isRecordAccessible(timestampList[i]))
			return i;
	}
	return -1;
}

primitive::timestamp_t BaseTransaction::getNewCommitTimestamp() {
	primitive::timestamp_t output;
	output = (primitive::timestamp_t)(client_.sessionState_->getNextEpoch() * client_.clientCnt_ + client_.clientID_);
	client_.sessionState_->advanceEpoch();
	return output;
}

std::string BaseTransaction::pointer_to_string(Timestamp* ts) const{
	std::ostringstream stream;
	for (size_t i = 0; i < config::tpcc_settings::VERSION_NUM; i++)
		stream << ts[i] << ", ";
	return stream.str();
}

std::string BaseTransaction::readTimestampToString() const{
	std::stringstream ss;
	for (size_t i = 0; i < client_.clientCnt_; i++)
		ss << "client " << i << ": " << client_.localTimestampVector_->getRegion()[i] << ", ";

	return ss.str();
}

uint64_t BaseTransaction::getOrderRID() const{
	return nextOrderID_;
}

uint64_t BaseTransaction::getNewOrderRID() const{
	return nextNewOrderID_;
}

uint64_t BaseTransaction::getOrderLineRID() const{
	return nextOrderLineID_;
}

uint64_t BaseTransaction::getHistoryRID() const{
	return nextHistoryID_;
}

void BaseTransaction::incrementOrderRID(size_t step){
	nextOrderID_ = (uint64_t)( (nextOrderID_ + step) % config::tpcc_settings::ORDER_BUFFER_PER_CLIENT);
}

void BaseTransaction::incrementNewOrderRID(size_t step){
	nextNewOrderID_ = (uint64_t)( (nextNewOrderID_ + step) % config::tpcc_settings::ORDER_BUFFER_PER_CLIENT);
}

void BaseTransaction::incrementOrderLineRID(size_t step){
	nextOrderLineID_ = (uint64_t)( (nextOrderLineID_ + step) % (config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) );
}

void BaseTransaction::incrementHistoryRID(size_t step){
	nextHistoryID_ = (uint64_t)( (nextHistoryID_ + step) % config::tpcc_settings::HISTORY_BUFFER_PER_CLIENT);
}

uint64_t BaseTransaction::reserveOrderLineRID(size_t orderLineCnt){
	if ( (uint64_t)( (nextOrderLineID_ + orderLineCnt) < (config::tpcc_settings::ORDER_BUFFER_PER_CLIENT * tpcc_settings::ORDER_MAX_OL_CNT) ))
		return nextOrderLineID_;
	else {
		// all the orderlines cannot fit into the table. so we start from the beginning.
		nextOrderLineID_ = 0;
		return nextOrderLineID_;
	}
}

void BaseTransaction::cleanupAfterCommit(){
	client_.oracleContext_->clearSnapshot();
}


} /* namespace TPCC */
