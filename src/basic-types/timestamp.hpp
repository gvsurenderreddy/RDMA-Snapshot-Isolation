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
#include "PrimitiveTypes.hpp"

class Timestamp {
private:
	primitive::timestamp_t timestamp_;
	primitive::client_id_t clientID_;
	primitive::lock_status_t lockStatus_;
	primitive::version_offset_t versionOffset_;

public:
	Timestamp();
	Timestamp(uint64_t ull);
	Timestamp(const primitive::lock_status_t lockStatus, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp);
	bool operator>(const Timestamp &right) const;
	const primitive::client_id_t getClientID() const;
	const primitive::timestamp_t getTimestamp() const;
	const primitive::version_offset_t getVersionOffset() const;
	const primitive::lock_status_t getLockStatus() const;
	void setClientID(const primitive::client_id_t clientID);
	void setTimestamp(const primitive::timestamp_t timestamp);
	void setAll(const primitive::lock_status_t lockStatus, const primitive::version_offset_t versionOffset, const primitive::client_id_t clientID, const primitive::timestamp_t timestamp);
	const uint64_t toUUL() const;
	void increment();
	const bool isEqual(const Timestamp& ts) const;
	void copy(const Timestamp& original);

	friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts) {
		os << "t:" << (int)ts.getTimestamp() << "|c:" << (int)ts.getClientID() << "|l:" << (int)ts.getLockStatus() << "|o:" << (int)ts.getVersionOffset() << "|"   ;
		return os;
	}
};

#endif // TIMESTAMP_HPP_
