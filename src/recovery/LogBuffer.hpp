/*
 * LogBuffer.hpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#ifndef SRC_RECOVERY_LOGBUFFER_HPP_
#define SRC_RECOVERY_LOGBUFFER_HPP_

#include "../rdma-region/RDMAContext.hpp"
#include "../rdma-region/RDMARegion.hpp"

class LogBuffer {
private:
	static const char code[16];

public:
	static const size_t serializedTimestampCharCnt;

	RDMARegion<char> *journal;

	LogBuffer(std::ostream &os, size_t clientsCnt, RDMAContext &context);
	~LogBuffer();

	static void serializeUnsignedINT32_t(char *buff, uint32_t number);
	static size_t getEntrySize(size_t clientsCnt);
	static size_t getOffsetOfTrxOutcome(size_t clientsCnt);

	LogBuffer& operator=(const LogBuffer&) = delete;	// Disallow copying
	LogBuffer(const LogBuffer&) = delete;				// Disallow copying

private:
	std::ostream &os_;
};

#endif /* SRC_RECOVERY_LOGBUFFER_HPP_ */
