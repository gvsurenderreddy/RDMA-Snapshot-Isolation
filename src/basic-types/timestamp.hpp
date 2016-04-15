/*
 *	timestamp.hpp
 *
 *	Created on: 28.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef TIMESTAMP_HPP_
#define TIMESTAMP_HPP_

#include <cstdint>
#include <iostream>
#include <string>
#include "PrimitiveTypes.hpp"

class Timestamp {
private:
	primitive::timestamp_t timestamp_;
	primitive::client_id_t clientID_;
	primitive::lock_status_t lockStatus_;
	primitive::version_offset_t versionOffset_;

	const uint8_t checkAliveBits = 0x0F;
	const uint8_t checkDeletedBits = 0x1F;
	const uint8_t deletingBits = 0x10;

	const uint8_t checkLockedBits = 0xF1;
	const uint8_t checkUnLockedBits = 0xF0;
	const uint8_t lockingBits = 0x01;
	const uint8_t unLockingBits = 0xF0;


public:
	Timestamp();
	Timestamp(uint64_t ull);
	Timestamp(const bool isDeleted, const bool isLocked, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp);
	bool operator>(const Timestamp &right) const;
	const primitive::client_id_t getClientID() const;
	const primitive::timestamp_t getTimestamp() const;
	const primitive::version_offset_t getVersionOffset() const;
	const bool isLocked() const;
	const bool isDeleted() const;

	void setClientID(const primitive::client_id_t clientID);
	void setTimestamp(const primitive::timestamp_t timestamp);
	void markDeleted();
	void markLocked();
	void markUnlocked();
	void setAll(const bool isDeleted, const bool isLocked, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp);

	const uint64_t toUUL() const;
	std::string serializeAsBinary() const;

	void increment();
	const bool isEqual(const Timestamp& ts) const;
	void copy(const Timestamp& original);

	friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts) {
		os
		<< "t:" << (int)ts.getTimestamp()
		<< "|c:" << (int)ts.getClientID()
		<< "|d:" << ((ts.isDeleted() ? "Y" : "N"))
		<< "|l:" << ((ts.isLocked() ? "Y" : "N"))
		<< "|o:" << (int)ts.getVersionOffset() << "|";
		return os;
	}
};

#endif // TIMESTAMP_HPP_
