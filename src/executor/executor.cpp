/*
 * executor.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

//#include "../TSM-SI/server/RDMAServer.hpp"
//#include "../TSM-SI/timestamp-oracle/TimestampServer.hpp"
//#include "../TSM-SI/client/RDMAClient.hpp"
#include "../benchmarks/TPC-C/server/TPCCServer.hpp"
#include "../benchmarks/TPC-C/client/TPCCClient.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>		// getopt()
#include <vector>
#include <thread>
#include <chrono>         // std::chrono::seconds


int main (int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "At least two arguments are needed" << std::endl;
		return 1;
	}

	if (strcmp(argv[1], "client")  == 0) {
		unsigned instanceNum = 0;
		uint8_t ibPort = 1;
		TPCCClient client(instanceNum, ibPort);
	}
	else if(strcmp(argv[1], "server") == 0) {

		uint32_t serverNum = 0;
		unsigned instance_num = 1;
		uint32_t cients_cnt = 1;
		TPCCServer server(serverNum, instance_num, cients_cnt);
	}


	/*
	if (strcmp(argv[1], "client")  == 0) {
		if (argc != 4 || strcmp(argv[2], "-i") != 0 ) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " client -i INSTANCE_NUM" << std::endl;
			std::cerr << "connects to server(s) specified in the config file" << std::endl;
			return 1;
		}
		unsigned instance_num = atoi(argv[3]);
		RDMAClient client(instance_num);
		client.start_client();
	}
	else if(strcmp(argv[1], "server") == 0) {
		if (argc != 7 || strcmp(argv[3], "-i") != 0 || strcmp(argv[5], "-n") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " server <s = server_num> -i INTANCE_NUM -n NUM_OF_CLIENTS" << std::endl;
			std::cerr << "starts a server and waits for connection on port Config.TCP_PORT[s]" << std::endl;
			std::cerr << "(valid range of s: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
			return 1;
		}
		unsigned instance_num = atoi(argv[4]);
		uint32_t cients_cnt = atoi(argv[6]);
		RDMAServer server(atoi(argv[2]), instance_num, cients_cnt);
		server.start_server();
	}
	else if(strcmp(argv[1], "timestamp") == 0) {
		if (argc != 4 || strcmp(argv[2], "-n") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " timestamp  -n NUM_OF_CLIENTS" << std::endl;
			std::cerr << "connects to the timestamp server specified in the config file" << std::endl;
			return 1;
		}
		uint32_t cients_cnt = atoi(argv[3]);
		TimestampServer tsServer(cients_cnt);
		tsServer.start_server();
	}
	else if(strcmp(argv[1], "instance") == 0) {
		if (argc != 7 || strcmp(argv[3], "-n") != 0 || strcmp(argv[5], "-s") != 0) {
			std::cerr << "Usage:" << std::endl;
			std::cerr << argv[0] << " instance <i = instance_num> -n NUM_OF_CLIENTS -s NUM_OF_SERVERS" << std::endl;
			std::cerr << "runs an RSI instance with the specified number of clients and servers." << std::endl;
			return 1;
		}
		unsigned instance_num = atoi(argv[2]);
		uint32_t clients_cnt = atoi(argv[4]);
		uint32_t servers_cnt = atoi(argv[6]);

		std::cout << "Running instance #" << (int)instance_num
				<< " with " << (int)clients_cnt << " clients and " << (int)servers_cnt<< " servers." << std::endl;


		std::vector<RDMAClient*> clients;
		std::vector<RDMAServer*> servers;

		std::vector<std::thread> client_threads;
		std::vector<std::thread> server_threads;

		for (unsigned i=0; i < servers_cnt; i++){
			RDMAServer *server = new RDMAServer(i, instance_num, clients_cnt);
			servers.push_back(server);
			server_threads.push_back(std::thread(&RDMAServer::start_server, std::ref(*server)));
		}

		for (unsigned i=0; i < clients_cnt; i++) {
			RDMAClient *client = new RDMAClient(instance_num);
			clients.push_back(client);
			client_threads.push_back(std::thread(&RDMAClient::start_client, std::ref(*client)));
		}

		std::cout << "synchronizing client threads...\n";
		for (auto& th : client_threads) th.join();

		std::cout << "synchronizing server threads...\n";
		for (auto& th : server_threads) th.join();

		for (auto& s : servers) delete s;
		for (auto& c : clients) delete c;
		std::cout << "FINISH" << std::endl;
	}
	else {
		std::cerr << "Error in arguments" << std::endl;
		return 1;
	}
	 */
	return 0;
}
