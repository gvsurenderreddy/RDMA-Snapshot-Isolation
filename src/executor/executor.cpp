/*
 * executor.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

#include "../../config.hpp"
#include "../benchmarks/TPC-C/server/TPCCServer.hpp"
#include "../oracle/Oracle.hpp"
#include "../benchmarks/TPC-C/client/TPCCClient.hpp"
#include "../benchmarks/TPC-C/client/ClientGroup.hpp"
#include "../util/utils.hpp"
#include "../util/SimpleArgumentParser.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>		// getopt()
#include <vector>
#include <thread>
#include <chrono>		// std::chrono::seconds
#include <string>
#include <cmath>		// std::abs
#include <exception>
#include <cassert>
#include <functional>
#include <stdlib.h>		// srand, rand


#define CLASS_NAME "executor"

void checkConfigFile();
void print_usage(std::string executable_filename);
void print_error_client(std::string executable_filename);
void print_error_server(std::string executable_filename);
void print_error_oracle(std::string executable_filename);
void print_error_instance(std::string executable_filename);


int main (int argc, char *argv[]) {
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed

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

			uint16_t homeWarehouseID = (uint16_t)(instanceID * config::tpcc_settings::WAREHOUSE_PER_SERVER + rand() % config::tpcc_settings::WAREHOUSE_PER_SERVER);
			uint8_t homeDistrictID = (uint8_t)(rand() % config::tpcc_settings::DISTRICT_PER_WAREHOUSE);

			//			TPCC::ClientGroup clientGroup(instanceID, 1, homeWarehouseID, ibPort);
			//			clientGroup.start();
			//			clientGroup.join();
			TPCC::TPCCClient client(instanceID, homeWarehouseID, homeDistrictID, ibPort, NULL);
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
			uint32_t serverID = atoi(argv[2]);
			unsigned instanceID = (unsigned)serverID;
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
			uint32_t clientsCnt = std::stoi(argParser.getArgumentForOption("-nc"));
			uint32_t instancesCnt = std::stoi(argParser.getArgumentForOption("-ni"));
			Oracle ts(clientsCnt, instancesCnt);
		}
		catch(const std::exception& e){
			print_error_oracle(std::string(argv[0]));
			return 1;
		}
	}
	else if(argParser.hasOption("instance")){
		try{
			bool hasServer = false;
			uint32_t serverID = -1;

			assert(argParser.getArgumentForOption("-b") == "TPCC");
			unsigned instanceID = (uint32_t)std::stoi(argParser.getArgumentForOption("-i"));

			if (!argParser.hasOption("-w") && !argParser.hasOption("-s")) {
				print_error_instance(std::string(argv[0]));
				return 1;
			}

			if (argParser.hasOption("-s")) {
				hasServer = true;;
				serverID = (size_t)std::stoi(argParser.getArgumentForOption("-s"));
			}

			uint16_t homeWarehouseID;

			if (argParser.hasOption("-w"))
				homeWarehouseID = (uint16_t)std::stoi(argParser.getArgumentForOption("-w"));
			else
				homeWarehouseID = (uint16_t)(instanceID * config::tpcc_settings::WAREHOUSE_PER_SERVER + rand() % config::tpcc_settings::WAREHOUSE_PER_SERVER);

			uint32_t clientsCntInInstance = std::stoi(argParser.getArgumentForOption("-c"));
			uint32_t totalClientsCnt = std::stoi(argParser.getArgumentForOption("-n"));
			size_t portsCnt = (size_t)std::stoi(argParser.getArgumentForOption("-pc"));

			TPCC::TPCCServer *server;
			std::thread serverThread;

			if (hasServer){
				// running server
				server = new TPCC::TPCCServer(serverID, instanceID, totalClientsCnt);
				serverThread = std::thread(&TPCC::TPCCServer::start, std::ref(*server));
				sleep(10);	// sleep for some seconds so that (all other) servers become ready
			}

			// running clients
			if (clientsCntInInstance != 0){
				TPCC::ClientGroup clientGroup(instanceID, clientsCntInInstance, homeWarehouseID, portsCnt);
				clientGroup.start();
				clientGroup.join();
			}

			if (hasServer) {
				serverThread.join();
				delete server;
			}

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
	std::cerr << executable_filename << " #SERVER_ID -s #SERVER_ID -b #BENCHMARK_NAME -n #NUM_OF_CLIENTS" << std::endl;
	std::cerr << "starts a server and waits for connections" << std::endl;
	std::cerr << std::endl;
	std::cerr << '\t' << "-s  (required)" << '\t' << "valid range: [0, Config.SERVER_CNT - 1]" << std::endl;
	std::cerr << '\t' << "-b  (required)" << '\t' << "choice of benchmark (e.g. TPCC)" << std::endl;
	std::cerr << '\t' << "-n  (required)" << '\t' << "number of clients" << std::endl;
	std::cerr << "" << std::endl;
}
void print_error_oracle(std::string executable_filename) {
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " oracle -ni #NUM_OF_INSTANCES -nc #NUM_OF_CLIENTS" << std::endl;
	std::cerr << "connects to the oracle, specified in the config file" << std::endl;
	std::cerr << std::endl;
	std::cerr << '\t' << "-ni  (required)" << '\t' << "number of client instances" << std::endl;
	std::cerr << '\t' << "-nc  (required)" << '\t' << "number of clients" << std::endl;
}

void print_error_instance(std::string executable_filename) {
	std::cerr << "Usage:" << std::endl;
	std::cerr << executable_filename << " instance -i #INSTANCE_NUM -b #BENCHMARK_NAME -s #SERVER_ID -c #NUM_OF_INSTANCE_CLIENTS -n #NUM_OF_TOTAL_CLIENTS -pc #NUM_OF_IB_PORTS" << std::endl;
	std::cerr << "creates an instance with one server and clients" << std::endl;
	std::cerr << std::endl;
	std::cerr << '\t' << "-i  (required)" << '\t' << "unique identifier of the instance" << std::endl;
	std::cerr << '\t' << "-b  (required)" << '\t' << "choice of benchmark (e.g. TPCC)" << std::endl;
	std::cerr << '\t' << "-s  (optional)" << '\t' << "index of the server in the config file. Out of '-s' and '-w' options, once must be declared" << std::endl;
	std::cerr << "\t" << "-w  (optional)" << "\t" << "the home warehouse for clients (the default is one of the warehouses on #SERVER_ID)" << std::endl;
	std::cerr << '\t' << "-c  (required)" << '\t' << "number of clients in the instance" << std::endl;
	std::cerr << '\t' << "-n  (required)" << '\t' << "number of total clients in the experiment" << std::endl;
	std::cerr << '\t' << "-pc (required)" << '\t' << "number of IB ports" << std::endl;
}

