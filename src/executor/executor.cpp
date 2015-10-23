/*
 * executor.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

#include "../rdma-SI/server/RDMAServer.hpp"
#include "../rdma-SI/timestamp-oracle/TimestampServer.hpp"
#include "../rdma-SI/client/RDMAClient.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>		// getopt()


int main (int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "At least two arguments are needed" << std::endl;
		return 1;
	}
	if (strcmp(argv[1], "client")  == 0) {
		if (argc != 2) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " client" << std::endl;
			std::cerr << "connects to server(s) specified in the config file" << std::endl;
			return 1;
		}
		RDMAClient client;
		client.start_client();
	}
	else if(strcmp(argv[1], "server") == 0) {
		if (argc != 5 || strcmp(argv[3], "-n") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " server <i = server_num> -n NUM_OF_CLIENTS" << std::endl;
			std::cerr << "starts a server and waits for connection on port Config.TCP_PORT[i]" << std::endl;
			std::cerr << "(valid range of i: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
			return 1;
		}
		uint32_t cients_cnt = atoi(argv[4]);
		RDMAServer server(atoi(argv[2]), cients_cnt);
		server.start_server();
	}
	else if(strcmp(argv[1], "timestamp") == 0) {
		if (argc != 4 || strcmp(argv[3], "-n") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " timestamp  -n NUM_OF_CLIENTS" << std::endl;
			std::cerr << "connects to the timestamp server specified in the config file" << std::endl;
			return 1;
		}
		uint32_t cients_cnt = atoi(argv[3]);
		TimestampServer tsServer(cients_cnt);
		tsServer.start_server();
	}
	else {
		std::cerr << "Error in arguments" << std::endl;
		return 1;
	}
	return 0;
}
