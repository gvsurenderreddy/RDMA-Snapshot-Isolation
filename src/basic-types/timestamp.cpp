/*
 * Timestamp.cpp
 *
 *  Created on: Oct 9, 2015
 *      Author: erfanz
 */

#include "../basic-types/timestamp.hpp"

#include <bitset>
#include <climits>

Timestamp::Timestamp(): Timestamp(true, false, 0, 0, 0){
//	isLocked = false;
//	isDeleted = true;
//	clientID = 0;
//	timestamp = 0;
//	versionOffset = 0;
}

Timestamp::Timestamp(uint64_t ull) {
	timestamp_ = (primitive::timestamp_t) ( (ull << 32 ) >> 32);						// bits 1 - 32
	clientID_ = (primitive::client_id_t) ( (ull << 16) >> 48 );				// bits 33 - 48
	lockStatus_ = (primitive::lock_status_t) ( (ull << 8) >> 56);			// bits 49 - 56
	versionOffset_ = (primitive::version_offset_t) ( ull >> 56); 	// bits 57 - 64
}

Timestamp::Timestamp(const bool isDeleted, const bool isLocked, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp){
	if (isDeleted && isLocked) lockStatus_ = BitMasks::checkDeletedBits & BitMasks::checkLockedBits;
	else if (isDeleted && !isLocked) lockStatus_ = BitMasks::checkDeletedBits & BitMasks::checkUnLockedBits;
	else if (!isDeleted && isLocked) lockStatus_ = BitMasks::checkAliveBits & BitMasks::checkLockedBits;
	else lockStatus_ = BitMasks::checkAliveBits & BitMasks::checkUnLockedBits;
	timestamp_ = timestamp;
	clientID_ = clientID;
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

const bool Timestamp::isLocked() const{
	return ((lockStatus_ & BitMasks::checkLockedBits) << 7 ) >> 7  == 1;
}

const bool Timestamp::isDeleted() const{
	return (lockStatus_ & BitMasks::checkDeletedBits) >> 4  == 1;
}

void Timestamp::setClientID(const primitive::client_id_t clientID) {
	this->clientID_ = clientID;
}

void Timestamp::setTimestamp(const primitive::timestamp_t timestamp) {
	this->timestamp_ = timestamp;
}

void Timestamp::markDeleted(){
	lockStatus_ = lockStatus_ | BitMasks::deletingBits;
}

void Timestamp::markLocked(){
	lockStatus_ = lockStatus_ | BitMasks::lockingBits;
}

void Timestamp::markUnlocked(){
	lockStatus_ = lockStatus_ & BitMasks::unLockingBits;
}

void Timestamp::setAll(const bool isDeleted, const bool isLocked, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp){
	if (isDeleted && isLocked) lockStatus_ = BitMasks::checkDeletedBits & BitMasks::checkLockedBits;
	else if (isDeleted && !isLocked) lockStatus_ = BitMasks::checkDeletedBits & BitMasks::checkUnLockedBits;
	else if (!isDeleted && isLocked) lockStatus_ = BitMasks::checkAliveBits & BitMasks::checkLockedBits;
	else lockStatus_ = BitMasks::checkAliveBits & BitMasks::checkUnLockedBits;

	versionOffset_ = versionOffset;
	clientID_ = clientID;
	timestamp_ = timestamp;
}

const uint64_t Timestamp::toUUL() const {
    std::string str = serializeAsBinary();
    std::bitset<CHAR_BIT * sizeof(Timestamp)> bitRep(str);
    return bitRep.to_ullong();
}

std::string Timestamp::serializeAsBinary() const {
    std::string str("");
    const unsigned char *p = reinterpret_cast<const unsigned char *>(this);
    for(size_t s = 0; s < sizeof(Timestamp); ++s, ++p) {
        // Code to serialize one byte
        std::bitset<CHAR_BIT> x(*p);
        str += x.to_string();
    }
    return str;
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
