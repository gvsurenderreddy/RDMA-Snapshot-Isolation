/*
 * executor.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

//#include "../TSM-SI/server/RDMAServer.hpp"
//#include "../TSM-SI/timestamp-oracle/TimestampServer.hpp"
//#include "../TSM-SI/client/RDMAClient.hpp"
#include "../../config.hpp"
#include "../benchmarks/TPC-C/server/TPCCServer.hpp"
#include "../oracle/Oracle.hpp"
#include "../benchmarks/TPC-C/client/TPCCClient.hpp"
#include "../util/argumentParser.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>		// getopt()
#include <vector>
#include <thread>
#include <chrono>         // std::chrono::seconds
#include <string>
#include <cmath>		// std::abs
#include <exception>
#include <cassert>
#include <functional>


#define CLASS_NAME "executor"

void checkConfigFile();
void print_usage(std::string executable_filename);
void print_error_client(std::string executable_filename);
void print_error_server(std::string executable_filename);
void print_error_oracle(std::string executable_filename);
void print_error_instance(std::string executable_filename);


int main (int argc, char *argv[]) {
	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		print_usage(std::string(argv[0]));
		return 1;
	}

	checkConfigFile();

	SimpleArgumentParser argParser(argc, argv);

	if (argParser.hasOption("client")){
		try{
			assert(argParser.getArgumentForOption("-b") == "TPCC");
			uint8_t ibPort = (uint8_t)std::stoi(argParser.getArgumentForOption("-p"));
			unsigned instanceID = std::stoi(argParser.getArgumentForOption("-i"));
			TPCC::TPCCClient client(instanceID, ibPort);
			client.start();
		}
		catch(const std::exception& e){
			print_error_client(std::string(argv[0]));
			return 1;
		}
	}
	else if (argParser.hasOption("server")){
		try{
			assert(argParser.getArgumentForOption("-b") == "TPCC");
			unsigned instanceID = 1;
			uint32_t serverID = atoi(argv[2]);
			uint32_t clientsCnt = std::stoi(argParser.getArgumentForOption("-n"));
			TPCC::TPCCServer server(serverID, instanceID, clientsCnt);
			server.start();
		}
		catch(const std::exception& e){
			print_error_server(std::string(argv[0]));
			return 1;
		}
	}
	else if (argParser.hasOption("oracle")){
		try{
			uint32_t clientsCnt = std::stoi(argParser.getArgumentForOption("-n"));
			Oracle ts(clientsCnt);
		}
		catch(const std::exception& e){
			print_error_oracle(std::string(argv[0]));
			return 1;
		}
	}
	else if(argParser.hasOption("instance")){
		try{
			assert(argParser.getArgumentForOption("-b") == "TPCC");
			unsigned instanceID = (uint32_t)std::stoi(argParser.getArgumentForOption("-i"));
			uint32_t serverID = (size_t)std::stoi(argParser.getArgumentForOption("-s"));
			uint32_t clientsCnt = std::stoi(argParser.getArgumentForOption("-n"));
			size_t portsCnt = (size_t)std::stoi(argParser.getArgumentForOption("-pc"));

			// running server
			TPCC::TPCCServer server(serverID, instanceID, clientsCnt);
			std::thread serverThread(&TPCC::TPCCServer::start, &server);

			sleep(10);	// sleep for some seconds so that (all other) servers become ready

			// running clients
			std::vector<TPCC::TPCCClient *> clients(clientsCnt);
			std::vector<std::thread> clientsThreads;
			for (uint32_t i = 0; i < clientsCnt; i++) {
				uint8_t ibPort = (uint8_t)(i % portsCnt + 1);
				clients[i] = new TPCC::TPCCClient(instanceID, ibPort);
				clientsThreads.push_back(std::thread(&TPCC::TPCCClient::start, std::ref(*clients[i])));
			}

			// waiting for threads to finish and join
			for (auto& th : clientsThreads) th.join();
			serverThread.join();

			// cleanup
			for (auto& c : clients) delete c;

			std::cout << "All clients and server threads are finished." << std::endl;

		}
		catch(const std::exception& e){
			print_error_instance(std::string(argv[0]));
			return 1;
		}
	}
	else{
		print_usage(std::string(argv[0]));
	}
	return 0;
}

void checkConfigFile() {
	// check if different ratios in the transaction mix are correct.
	double eps = 0.00001;
	double d = 0;

	for (unsigned i = 0; i < config::tpcc_settings::TRANSACTION_MIX_RATIOS.size(); i++)
		d += config::tpcc_settings::TRANSACTION_MIX_RATIOS.at(i);

	if (std::abs(d - 1) > eps) {
		PRINT_CERR(CLASS_NAME, __func__, "Transactions' ratios in the config file do not add up to 1 (Currently it's " << d << ")");
		exit(-1);
	}
}

void print_usage(std::string executable_filename) {
	std::cerr << "3 different usages of the executor:" << std::endl;
	std::cerr << std::endl;
	print_error_client(executable_filename);
	std::cerr << std::endl;
	print_error_server(executable_filename);
	std::cerr << std::endl;
	print_error_oracle(executable_filename);
	std::cerr << std::endl;
}

void print_error_client(std::string executable_filename) {
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " client -b #BENCHMARK_NAME -p #PORT_NUM -i #INSTANCE_NUM" << std::endl;
	std::cerr << "starts a client and runs the BENCHMARK (e.g. TPCC)" << std::endl;
}
void print_error_server(std::string executable_filename){
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " #SERVER_ID -b #BENCHMARK_NAME -n #NUM_OF_CLIENTS" << std::endl;
	std::cerr << "starts a server and waits for connection on port Config.TCP_PORT[s] to run the BENCHMARK (e.g. TPCC) " << std::endl;
	std::cerr << "(valid range of SERVER_ID: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
}
void print_error_oracle(std::string executable_filename) {
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " oracle -n #NUM_OF_CLIENTS" << std::endl;
	std::cerr << "connects to the oracle, specified in the config file" << std::endl;
}

void print_error_instance(std::string executable_filename) {
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " instance -i #INSTANCE_NUM -b #BENCHMARK_NAME -s #SERVER_ID -n #NUM_OF_CLIENTS -pc #NUM_OF_IB_PORTS" << std::endl;
	std::cerr << "creates an instance with one server and clients" << std::endl;
	std::cerr << '\t' << "#INSTANCE_NUM:" << '\t' << "unique identifier of the instance" << std::endl;
	std::cerr << '\t' << "#BENCHMARK_NAME:" << '\t' << "e.g. TPCC" << std::endl;
	std::cerr << '\t' << "#SERVER_ID:" << '\t' << "index of the server in the config file" << std::endl;
	std::cerr << '\t' << "#NUM_OF_CLIENTS:" << '\t' << "number of clients in the instance" << std::endl;
	std::cerr << '\t' << "#NUM_OF_IB_PORTS:" << '\t' << "number of IB ports" << std::endl;
}
