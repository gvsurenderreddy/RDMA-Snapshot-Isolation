/*
 * Snapshot.cpp
 *
 *  Created on: May 19, 2016
 *      Author: erfanz
 */

#include "Snapshot.hpp"

Snapshot::Snapshot() {
	// TODO Auto-generated constructor stub
}

Snapshot::~Snapshot() {
	// TODO Auto-generated destructor stub
}

bool Snapshot::isRecordVisible(const Timestamp& record_ts) const{
	if (record_ts.isLocked() || record_ts.isDeleted())
		// item is already locked or deleted
		return false;

//	primitive::client_id_t committingClient = ts.getClientID();
//	if (committingClient == client_.clientID_)
//		// regardless of whether the version matches the snapshot or not, the version is installed by the client itself, so is valid
//		// when is it useful? when using adaptive abort rate control.
//		return true;


//	return record_ts.getTimestamp() <= client_.localTimestampVector_->getRegion()[committingClient];
	return true;
}
