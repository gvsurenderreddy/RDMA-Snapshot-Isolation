/*
 *	BaseTradTrxManager.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: erfanz
 */

#include "BaseTradTrxManager.hpp"
#include "../util/utils.hpp"
#include <stdio.h>
#include <string.h>
#include <sstream>	// for stringstream
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

//OrdersVersion*		TradTrxManager::orders_region		= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
//OrderLineVersion*		TradTrxManager::order_line_region	= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
//CCXactsVersion*		TradTrxManager::cc_xacts_region		= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];

//OrdersVersion*		TradTrxManager::orders_region		= new OrdersVersion[MAX_BUFFER_SIZE];		// TODO
//OrderLineVersion*	TradTrxManager::order_line_region	= new OrderLineVersion[MAX_BUFFER_SIZE];	// TODO
//CCXactsVersion*		TradTrxManager::cc_xacts_region		= new CCXactsVersion[MAX_BUFFER_SIZE];		// TODO

int	BaseTradTrxManager::server_sockfd	= -1;		// Server's socket file descriptor
int	BaseTradTrxManager::res_mng_socks[SERVER_CNT];

Timestamp	BaseTradTrxManager::timestamp;
std::mutex 	BaseTradTrxManager::timestamp_mutex;


int BaseTradTrxManager::acquire_commit_timestamp(Timestamp *commit_timestamp) {
	timestamp_mutex.lock();
	timestamp.value++;
	memcpy(commit_timestamp, &(BaseTradTrxManager::timestamp), sizeof(Timestamp));
	timestamp_mutex.unlock();
}


std::string BaseTradTrxManager::get_full_desc(BaseContext &ctx) {
	std::stringstream sstm;
	//sstm << "ip: " << ctx->client_ip << ", port: " << ctx->client_port  << ", sock: " << ctx->client_sockfd; 
	sstm << ctx.sockfd;
	return sstm.str();
}

int BaseTradTrxManager::destroy_resources () {
	//for (int i = 0; i < SERVER_CNT; i++)
	//	if (res_mng_socks[i] >= 0)
	//		TEST_NZ (close (res_mng_socks[i]));
	
	//if (s_ctx.ib_ctx)
	//	TEST_NZ (ibv_close_device (s_ctx.ib_ctx));
	
	close(server_sockfd);	// close the socket
	return 0;
}

int BaseTradTrxManager::initialize_data_structures(){
	timestamp.value = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("[Info] Timestamp set to " << timestamp.value);
	return 0;
}

void BaseTradTrxManager::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << "[IPTradTrxManager | IBTradTrxManager] starts a server and wait for connection" << std::endl;
}