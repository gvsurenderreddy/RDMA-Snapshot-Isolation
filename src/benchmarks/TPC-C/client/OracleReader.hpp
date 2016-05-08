/*
 * OracleReader.hpp
 *
 *  Created on: May 6, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_CLIENT_ORACLEREADER_HPP_
#define SRC_BENCHMARKS_TPC_C_CLIENT_ORACLEREADER_HPP_

#include "../../../basic-types/PrimitiveTypes.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../../../rdma-region/RDMARegion.hpp"
#include "../../../oracle/OracleContext.hpp"
#include <vector>
#include <mutex>		// for std::lock_guard

namespace TPCC {

class OracleReader {
private:
	std::ostream *os_;
	unsigned instanceID_;
	size_t	clientCnt_;
	RDMAContext *context_;
	OracleContext *oracleContext_;
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;
	size_t liveClientCnt_;
	std::mutex mutex_;

public:
	OracleReader(unsigned instanceID, uint32_t clientsCnt, uint8_t ibPort);
	RDMARegion<primitive::timestamp_t> *getTimestampVectorRegion() const;
	void informAboutFinishing();
	void connectToOracle();
	void start();
	void disconnectFromOracle();
	virtual ~OracleReader();

};

} /* namespace TPCC */

#endif /* SRC_BENCHMARKS_TPC_C_CLIENT_ORACLEREADER_HPP_ */
