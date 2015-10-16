/*
 *	timestamp.hpp
 *
 *	Created on: 28.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef TIMESTAMP_HPP_
#define TIMESTAMP_HPP_

#include <cstdint>

class Timestamp {
public:
	uint64_t value;
	bool operator>(const Timestamp &right) const;
};


#endif // TIMESTAMP_HPP_
