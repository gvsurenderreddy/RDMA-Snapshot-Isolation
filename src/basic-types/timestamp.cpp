/*
 * Timestamp.cpp
 *
 *  Created on: Oct 9, 2015
 *      Author: erfanz
 */

#include "../basic-types/timestamp.hpp"

Timestamp::Timestamp(){
	timestamp_ = 0;
	clientID_ = 0;
	lockStatus_ = 0;
	versionOffset_ = 0;
}

Timestamp::Timestamp(uint64_t ull) {
//	timestamp_ = (primitive::timestamp_t) (ull >> 32);						// bits 1 - 32
//	clientID_ = (primitive::client_id_t) ( (ull << 32) >> 48 );				// bits 33 - 48
//	lockStatus_ = (primitive::lock_status_t) ( (ull << 48) >> 56);			// bits 49 - 56
//	versionOffset_ = (primitive::version_offset_t) ( (ull << 56) >> 56); 	// bits 57 - 64

	timestamp_ = (primitive::timestamp_t) ( (ull << 32 ) >> 32);						// bits 1 - 32
	clientID_ = (primitive::client_id_t) ( (ull << 16) >> 48 );				// bits 33 - 48
	lockStatus_ = (primitive::lock_status_t) ( (ull << 8) >> 56);			// bits 49 - 56
	versionOffset_ = (primitive::version_offset_t) ( ull >> 56); 	// bits 57 - 64
}

Timestamp::Timestamp(const primitive::lock_status_t lockStatus, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp){
	timestamp_ = timestamp;
	clientID_ = clientID;
	lockStatus_ = lockStatus;
	versionOffset_ = versionOffset;
}

bool Timestamp::operator>(const Timestamp &right) const {
	return this->timestamp_ > right.timestamp_;
}

const primitive::client_id_t Timestamp::getClientID() const {
	return clientID_;
}
const primitive::timestamp_t Timestamp::getTimestamp() const {
	return timestamp_;
}
const primitive::version_offset_t Timestamp::getVersionOffset() const {
	return versionOffset_;
}
const primitive::lock_status_t Timestamp::getLockStatus() const {
	return lockStatus_;
}

void Timestamp::setClientID(const primitive::client_id_t clientID) {
	this->clientID_ = clientID;
}

void Timestamp::setTimestamp(const primitive::timestamp_t timestamp) {
	this->timestamp_ = timestamp;
}

void Timestamp::setAll(const primitive::lock_status_t lockStatus, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp){
	lockStatus_ = lockStatus;
	versionOffset_ = versionOffset;
	clientID_ = clientID;
	timestamp_ = timestamp;
}

const uint64_t Timestamp::toUUL() const {
	uint64_t uul;
	uul = (uint64_t)timestamp_;
	uul = uul << 32;
	uul += (uint64_t)clientID_;
	uul = uul << 16;
	uul += (uint64_t)lockStatus_;
	uul = uul << 8;
	uul += (uint64_t)versionOffset_;
	return uul;
}

void Timestamp::increment() {
	timestamp_++;
}

const bool Timestamp::isEqual(const Timestamp& ts) const {
	if (
			this->lockStatus_ == ts.lockStatus_ &&
			this->versionOffset_ == ts.versionOffset_ &&
			this->clientID_ == ts.clientID_ &&
			this->timestamp_ == ts.timestamp_)
		return true;
	else return false;
}

void Timestamp::copy(const Timestamp& original) {
	this->lockStatus_ = original.lockStatus_;
	this->versionOffset_ = original.versionOffset_;
	this->clientID_ = original.clientID_;
	this->timestamp_ = original.timestamp_;
}
