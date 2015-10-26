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
#include <getopt.h>

void print_client_usage(char* first_arg) {
	std::cout << "Usage:" << std::endl << std::endl;
	std::cout << first_arg << " client -v VERB_TYPE -m MEMORY_ACCESS" << std::endl;
	std::cout << "\tAcceptable values for VERB_TYPE are: READ, WRITE, CAS, FA" << std::endl;
	std::cout << "\tAcceptable values for MEMORY_ACCESS are: SHARED, DEDICATED" << std::endl << std::endl;
}

void print_server_usage(char* first_arg) {
	std::cerr << "Usage:" << std::endl << std::endl;
	std::cerr << first_arg << " server -c NumOfClients" << std::endl;
	std::cerr << "\tConnects to the first server specified in the general config file and runs the benchmark" << std::endl << std::endl;
}


int main (int argc, char *argv[]) {
	benchmark_config::VERB_TYPE_ENUM verb_type;
	benchmark_config::MEMORY_ACCESS_ENUM memory_access;

	if (argc < 2) {
		print_client_usage(argv[0]);
		std::cout << "----- OR -----" << std::endl;
		print_server_usage(argv[0]);

		return 1;
	}
	if (strcmp(argv[1], "client")  == 0) {
		if (argc != 6 || strcmp(argv[2], "-v")!=0 || strcmp(argv[4], "-m")!=0) {
			print_client_usage(argv[0]);
			return 1;
		}

		if (strcmp(argv[3], "READ") == 0)
			verb_type = benchmark_config::VERB_TYPE_ENUM::READ;
		else if (strcmp(argv[3], "WRITE") == 0)
			verb_type = benchmark_config::VERB_TYPE_ENUM::WRITE;
		else if (strcmp(argv[3], "CAS") == 0)
			verb_type = benchmark_config::VERB_TYPE_ENUM::CAS;
		else if (strcmp(argv[3], "FA") == 0)
			verb_type = benchmark_config::VERB_TYPE_ENUM::FA;
		else {
			print_client_usage(argv[0]);
			exit(1);
		}

		if (strcmp(argv[5], "SHARED") == 0)
			memory_access = benchmark_config::MEMORY_ACCESS_ENUM::SHARED;
		else if (strcmp(argv[5], "DEDICATED") == 0)
			memory_access = benchmark_config::MEMORY_ACCESS_ENUM::DEDICATED;
		else {
			print_client_usage(argv[0]);
			exit(1);
		}
		BenchmarkClient client(verb_type, memory_access);
		client.start_client();
	}
	else if(strcmp(argv[1], "server") == 0) {
		if (argc != 4 || strcmp(argv[2], "-c") != 0) {
			print_server_usage(argv[0]);
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
