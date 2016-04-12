/*
 * SessionState.hpp
 *
 *  Created on: Feb 26, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_SESSIONSTATE_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_SESSIONSTATE_HPP_

#include "../../../basic-types/PrimitiveTypes.hpp"

class SessionState{
private:
	const uint16_t			homeWarehouseID_;
	const uint8_t			homeDistrictID_;
	primitive::timestamp_t	nextEpoch_;
public:
	SessionState(const uint16_t homeWarehouseID, const uint8_t homeDistrictID, primitive::timestamp_t initialEpoch) :
		homeWarehouseID_(homeWarehouseID),
		homeDistrictID_(homeDistrictID),
		nextEpoch_(initialEpoch){}
	uint16_t getHomeWarehouseID() const{
		return homeWarehouseID_;
	}

	uint8_t getHomeDistrictID() const {
		return homeDistrictID_;
	}

	primitive::timestamp_t getNextEpoch() const{
		return nextEpoch_;
	}

	void advanceEpoch(){
		nextEpoch_++;
	}
};

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_SESSIONSTATE_HPP_ */
