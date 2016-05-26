/*
 * Snapshot.cpp
 *
 *  Created on: May 19, 2016
 *      Author: erfanz
 */

#include "AbstractSnapshot.hpp"

AbstractSnapshot::AbstractSnapshot() {
	// TODO Auto-generated constructor stub

}

AbstractSnapshot::~AbstractSnapshot() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream& os, const AbstractSnapshot& snapshot){
	return snapshot.print(os);
}

