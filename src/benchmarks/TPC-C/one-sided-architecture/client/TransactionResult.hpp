/*
 * TransactionResult.hpp
 *
 *  Created on: Mar 14, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_TRANSACTIONRESULT_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_TRANSACTIONRESULT_HPP_

#include "../../../../basic-types/PrimitiveTypes.hpp"
namespace TPCC{

struct TransactionResult {
	enum Result {
		COMMITTED,
		ABORTED
	} result;
	enum Reason {
		SUCCESS,
		INCONSISTENT_SNAPSHOT,
		UNSUCCESSFUL_LOCK
	} reason;

	struct Statistics{
		double indexElapsedMicroSec = 0;
		double executionPhaseMicroSec = 0;
		double checkVersionsPhaseMicroSec = 0;
		double lockPhaseMicroSec = 0;
		double updatePhaseMicroSec = 0;
		double commitSnapshotMicroSec = 0;
	} statistics;
	primitive::timestamp_t cts;
};
}

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_TRANSACTIONRESULT_HPP_ */
