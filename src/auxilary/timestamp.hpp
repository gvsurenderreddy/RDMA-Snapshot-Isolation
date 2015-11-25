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

class Timestamp {
private:
	//uint64_t value;
	uint32_t cID_;
	uint16_t pointer_;
	uint16_t lockStatus_;

public:
	Timestamp();
	Timestamp(uint64_t ull);
	Timestamp(const uint16_t lockStatus, const uint16_t pointer, const uint32_t cID);
	bool operator>(const Timestamp &right) const;
	const uint32_t getCID() const;
	const uint16_t getPointer() const;
	const uint16_t getLockStatus() const;
	void setCID(const uint32_t cID);
	void setAll(const uint16_t lockStatus, const uint16_t pointer, const uint32_t cID);
	const uint64_t toUUL() const;
	void increment();
	const bool isEqual(const Timestamp& ts) const;
	void copy(const Timestamp& original);

	friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts) {
		os << ts.getLockStatus() << "|" << ts.getPointer() << "|" << ts.getCID();
		return os;
	}
};

#endif // TIMESTAMP_HPP_
