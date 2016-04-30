/*
 * LogBuffer.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#include "LogBuffer.hpp"
#include "../basic-types/PrimitiveTypes.hpp"
#include "../../config.hpp"
#include <cstring>


#define CLASS_NAME "LogBuffer"

const char LogBuffer::code[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
const size_t LogBuffer::serializedTimestampCharCnt = sizeof(primitive::timestamp_t) * 2;


LogBuffer::LogBuffer(std::ostream &os, size_t clientsCnt, RDMAContext &context)
: os_(os){
	journal = new RDMARegion<char>(config::recovery_settings::ENTRY_PER_LOG_JOURNAL * getEntrySize(clientsCnt), context, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	std::memset(journal->getRegion(), 0, journal->getRegionSizeInByte());
}

LogBuffer::~LogBuffer(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");
	delete journal;
}

void LogBuffer::serializeUnsignedINT32_t(char buff[serializedTimestampCharCnt], uint32_t number) {
	for (int i = 0; i < 8; i++){
		unsigned ind = (unsigned)(number << (4 * i) >> (7 * 4));
		buff[i] = code[ind];
	}
}

size_t LogBuffer::getEntrySize(size_t clientsCnt){
	size_t snapshotSize = clientsCnt * serializedTimestampCharCnt;
	size_t trxOutcomeSize = 1;	// either C (for committed), or A (for aborted)
	return snapshotSize + config::recovery_settings::COMMAND_LOG_SIZE + trxOutcomeSize;
}

size_t LogBuffer::getOffsetOfTrxOutcome(size_t clientsCnt){
	size_t snapshotSize = clientsCnt * serializedTimestampCharCnt;
	return snapshotSize + config::recovery_settings::COMMAND_LOG_SIZE;
}
