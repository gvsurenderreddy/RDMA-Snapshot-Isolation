/*
 * RecoveryServer.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: erfanz
 */

#include "RecoveryServer.hpp"

#define CLASS_NAME "RecoveryServer"

RecoveryServer::RecoveryServer(std::ostream &os, size_t clientsCnt, RDMAContext &context)
: os_(os),
  clientsCnt_(clientsCnt){

	for (size_t c = 0; c < clientsCnt_; c++)
		logBuffers.push_back(new LogBuffer(os_, clientsCnt_, context));

	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Journals created.");
}

MemoryHandler<char> RecoveryServer::getMemoryHandler(primitive::client_id_t clientID){
	MemoryHandler<char> mh;
	logBuffers[clientID]->journal->getMemoryHandler(mh);
	return mh;
}

RecoveryServer::~RecoveryServer(){
	DEBUG_WRITE(os_, CLASS_NAME, __func__, "[Info] Deconstructor called ");
	for (size_t c = 0; c < clientsCnt_; c++)
		delete logBuffers[c];

}
