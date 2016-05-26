/*
 * Snapshot.hpp
 *
 *  Created on: May 19, 2016
 *      Author: erfanz
 */

#ifndef SRC_ORACLE_ABSTRACTSNAPSHOT_HPP_
#define SRC_ORACLE_ABSTRACTSNAPSHOT_HPP_

#include "../basic-types/timestamp.hpp"
#include <iostream>

class AbstractSnapshot {
protected:
	virtual std::ostream& print(std::ostream& os) const = 0;

public:
	AbstractSnapshot();
	virtual ~AbstractSnapshot();
	virtual bool isRecordVisible(const Timestamp& record_ts) const = 0;
	friend std::ostream& operator<<(std::ostream& os, const AbstractSnapshot& snapshot);
};

#endif /* SRC_ORACLE_ABSTRACTSNAPSHOT_HPP_ */
