/*
 * Snapshot.hpp
 *
 *  Created on: May 19, 2016
 *      Author: erfanz
 */

#ifndef SRC_ORACLE_VECTOR_SNAPSHOT_ORACLE_SNAPSHOT_HPP_
#define SRC_ORACLE_VECTOR_SNAPSHOT_ORACLE_SNAPSHOT_HPP_

#include "../AbstractSnapshot.hpp"

class Snapshot: public AbstractSnapshot {
public:
	Snapshot();
	virtual ~Snapshot();
	bool isRecordVisible(const Timestamp& record_ts) const;
};

#endif /* SRC_ORACLE_VECTOR_SNAPSHOT_ORACLE_SNAPSHOT_HPP_ */
