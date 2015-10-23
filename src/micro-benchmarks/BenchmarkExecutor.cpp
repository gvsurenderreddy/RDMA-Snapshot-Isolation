/*
 *
 * BenchmarkExecutor.cpp
 *
 *  Created on: Oct 20, 2015
 *      Author: Erfan Zamanian
 */

#include <iostream>
#include <cstring>
#include "simple-verbs/BenchmarkClient.hpp"
#include "simple-verbs/BenchmarkServer.hpp"


int main (int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "Usage:" << std::endl;
		std::cerr << argv[0] << " client" << std::endl;
		std::cerr << argv[0] << " server -n 2" << std::endl;
		return 1;
	}
	if (strcmp(argv[1], "client")  == 0) {
		if (argc != 2) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " client" << std::endl;
			std::cerr << "connects to the first server specified in the general config file and runs the benchmark" << std::endl;
			return 1;
		}
		BenchmarkClient client;
		client.start_client();
	}
	else if(strcmp(argv[1], "server") == 0) {
		if (argc != 4 || strcmp(argv[2], "-n") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " server -n NUM_OF_CLIENTS" << std::endl;
			std::cerr << "starts a server and waits for connection on port Config.TCP_PORT[0]" << std::endl;
			return 1;
		}
		uint32_t cients_cnt = atoi(argv[3]);
		BenchmarkServer server(cients_cnt);
		server.start_server();
	}
	else {
		std::cerr << "Error in arguments" << std::endl;
		return 1;
	}
	return 0;
}
