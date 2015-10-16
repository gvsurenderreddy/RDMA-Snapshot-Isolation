/*
 * Timestamp.cpp
 *
 *  Created on: Oct 9, 2015
 *      Author: erfanz
 */

#include "timestamp.hpp"

bool Timestamp::operator>(const Timestamp &right) const {
		// TODO: this should be changed to timestamp
		return this->value > right.value;
}
