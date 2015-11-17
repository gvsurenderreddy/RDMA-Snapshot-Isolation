/*
 * Timestamp.cpp
 *
 *  Created on: Oct 9, 2015
 *      Author: erfanz
 */

#include "timestamp.hpp"

Timestamp::Timestamp(){
	lockStatus_ = 0;
	pointer_ = 0;
	cID_ = 0;
}

Timestamp::Timestamp(uint64_t ull) {
	lockStatus_ = (uint16_t) (ull >> 48);
	pointer_ = (uint16_t) ( (ull << 16) >> 48);
	cID_ = (uint32_t) ( (ull << 32) >> 32 );
}

Timestamp::Timestamp(const uint16_t lockStatus, const uint16_t pointer, const uint32_t cID) {
	lockStatus_ = lockStatus;
	pointer_ = pointer;
	cID_ = cID;
}

bool Timestamp::operator>(const Timestamp &right) const {
	return this->cID_ > right.cID_;
}

const uint32_t Timestamp::getCID() const {
	return cID_;
}

const uint16_t Timestamp::getPointer() const {
	return pointer_;
}
const uint16_t Timestamp::getLockStatus() const {
	return lockStatus_;
}

void Timestamp::setCID(const uint32_t cID) {
	this->cID_ = cID;
}

void Timestamp::setAll(const uint16_t lockStatus, const uint16_t pointer, const uint32_t cID) {
	lockStatus_ = lockStatus;
	pointer_ = pointer;
	cID_ = cID;
}

const uint64_t Timestamp::toUUL() const {
	uint64_t uul;
	uul = lockStatus_;
	uul = uul << 16;
	uul += pointer_;
	uul = uul << 16;
	uul += cID_;
	return uul;
}

const bool Timestamp::isEqual(const Timestamp& ts) const {
	if (
			this->lockStatus_ == ts.lockStatus_ &&
			this->pointer_ == ts.pointer_ &&
			this->cID_ == ts.cID_)
		return true;
	else return false;
}

void Timestamp::copy(const Timestamp& original) {
	this->lockStatus_ = original.lockStatus_;
	this->pointer_ = original.pointer_;
	this->cID_ = original.cID_;
}



