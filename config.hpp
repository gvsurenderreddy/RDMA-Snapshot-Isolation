/*
 *	config.hpp
 *
 *  Created on: 27.Jan.2015
 *	Author: erfanz
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <stdint.h>


typedef enum {
	READ_SHIFT_WRITE,
	FIX_POINTER
} Insert_strategy;


static const uint16_t TCP_PORT			= 45679;			// the data sites are listening on this port
static const int IB_PORT				= 1;

static const char ITEM_FILENAME[50]		= "./data/item.data";

static const int CLIENTS_CNT 			= 6;

static const int TRANSACTION_CNT 		= 100000;

static const int MAX_ITEM_CNT			= 10000;		// Number of Items
static const int MAX_ORDERS_CNT			= TRANSACTION_CNT * CLIENTS_CNT;	// Number of Orders
static const int MAX_CCXACTS_CNT		= MAX_ORDERS_CNT;	// Number of CCXacts

static const int MAX_ITEM_VERSIONS		= 1;		// Maximum number of versions per data item
static const int MAX_ORDERS_VERSIONS	= 1;		// Maximum number of versions per data item
static const int MAX_ORDERLINE_VERSIONS	= 1;	
static const int MAX_CCXACTS_VERSIONS	= 1;		// Maximum number of versions per data item

static const int ORDERLINE_PER_ORDER	= 5;

static const int TIMEOUT_IN_MS			= 500;		/* ms */

static const int FETCH_BLOCK_SIZE		= 1;		// Number of items fetched at each RDMA READ 

// This parameter sets how many of the first runs should be excluded from the reported statistics
int NUM_OF_EXCLUDING_THE_FIRST_RUNS 	= 5000;	

static const Insert_strategy insert_strategy  = FIX_POINTER;

#endif /* CONFIG_H_ */