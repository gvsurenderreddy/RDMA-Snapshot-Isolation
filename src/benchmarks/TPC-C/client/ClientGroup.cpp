/*
 * ClientGroup.cpp
 *
 *  Created on: May 6, 2016
 *      Author: erfanz
 */

#include "ClientGroup.hpp"

#include "../../../../config.hpp"
#include "../../../util/utils.hpp"
#include <stdlib.h>		// srand, rand
#include <unistd.h>

#define CLASS_NAME "ClientGroup"

namespace TPCC {

ClientGroup::ClientGroup(unsigned instanceID, uint32_t clientsCnt, uint16_t homeWarehouseID, size_t ibPortsCnt)
: clientsCnt_(clientsCnt){

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Constructor called ");

	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed

	if (config::SNAPSHOT_CLUSTER_MODE == true){
		oracleReader_ = new OracleReader(instanceID, clientsCnt, 1);
	}

	for (uint32_t i = 0; i < clientsCnt; i++) {
		uint8_t ibPort = (uint8_t)(i % ibPortsCnt + 1);
		uint8_t homeDistrictID = (uint8_t)(rand() % config::tpcc_settings::DISTRICT_PER_WAREHOUSE);
		clients_.push_back(new TPCC::TPCCClient(instanceID, homeWarehouseID, homeDistrictID, ibPort, oracleReader_));
	}
}

void ClientGroup::start(){
	if (config::SNAPSHOT_CLUSTER_MODE == true)
		oracleReaderThread_ = new std::thread(&OracleReader::start, std::ref(oracleReader_));

	usleep(500);
	for (uint32_t i = 0; i < clientsCnt_; i++) {
		clientsThreads_.push_back(std::thread(&TPCC::TPCCClient::start, std::ref(*clients_[i])));
	}
}

void ClientGroup::join(){
	for (auto& th : clientsThreads_)
		th.join();
	if (config::SNAPSHOT_CLUSTER_MODE == true)
		oracleReaderThread_->join();
}

ClientGroup::~ClientGroup() {
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Deconstructor called ");
	for (auto& c : clients_)
		delete c;
	if (config::SNAPSHOT_CLUSTER_MODE == true)
		delete oracleReader_;
}

} /* namespace TPCC */
