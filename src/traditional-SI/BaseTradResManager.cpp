/*
 *	BaseTradResManager.hpp
 *
 *	Created on: 13.Feb.2015
 *	Author: Erfan Zamanian
 */


#include "BaseTradResManager.hpp"
#include "../util/utils.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>


ItemVersion*	BaseTradResManager::items_region	= new ItemVersion[ITEM_CNT * MAX_ITEM_VERSIONS];
std::mutex		BaseTradResManager::item_lock[ITEM_CNT];
int				BaseTradResManager::tcp_port;
int				BaseTradResManager::res_mng_sockfd;


int BaseTradResManager::destroy_resources () {
	delete[](BaseTradResManager::items_region);
	if (BaseTradResManager::res_mng_sockfd >= 0){
		TEST_NZ (close (BaseTradResManager::res_mng_sockfd));
	}
	return 0;
}

int BaseTradResManager::initialize_data_structures() {
	TEST_NZ (load_tables_from_files(items_region));
	DEBUG_COUT ("[Info] Tables loaded successfully");
	return 0;
}

void BaseTradResManager::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << "< IBTradResManager | IPTradResManager > <i = server_num>" << std::endl;
	std::cout << "starts a RM server and waits for connection on port Config.TCP_PORT[i]" << std::endl;
	std::cout << "(valid range of i: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
}

int BaseTradResManager::lock_item(int I_ID) {
	return item_lock[I_ID].try_lock();
}

int BaseTradResManager::unlock_item(int I_ID) {
	item_lock[I_ID].unlock();
	return 0;
}

int BaseTradResManager::decremenet_item_stock(int item_id, int quantity, Timestamp* commit_timestamp) {
	BaseTradResManager::items_region[item_id].write_timestamp	= commit_timestamp->value;
	BaseTradResManager::items_region[item_id].item.I_STOCK		-= quantity;
	
	if (BaseTradResManager::items_region[item_id].item.I_STOCK < 10)
		BaseTradResManager::items_region[item_id].item.I_STOCK += 20;
	
	return 0;
}