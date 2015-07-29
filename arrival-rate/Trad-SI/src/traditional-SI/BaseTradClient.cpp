/*
 *	BaseTradClient.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: erfanz
 */

#include "BaseTradClient.hpp"
#include "../util/utils.hpp"
#include "../util/zipf.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <time.h>	// for struct timespec


std::vector<int>		BaseTradClient::preselected_item_ids[4];

void BaseTradClient::fill_shopping_cart(struct Cart *cart, int sockfd) {
	int item_id;
	int quantity;
	
	DEBUG_COUT ("[Info] Contents of the cart (sock: " << sockfd << "): (Item ID,  Quantity)");
	for (int i=0; i < ORDERLINE_PER_ORDER; i++) {
		//item_id		= (i * ITEM_PER_SERVER) + (rand() % ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		item_id = preselected_item_ids[i].back();
		preselected_item_ids[i].pop_back();
		
		
		quantity	= (rand() % 5) + 1;			// the quanity of the item (not importatn)
		cart->cart_lines[i].SCL_I_ID	= item_id;
		cart->cart_lines[i].SCL_QTY		= quantity;
		DEBUG_COUT (".... " <<  item_id << '\t' << quantity);
	}
}


void BaseTradClient::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << "[IPTradClient | IBTradClient] connects to the transaction_manager server specified in Config.TRX_MANAGER_ADDR" << std::endl;
}

