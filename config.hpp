/*
 *	config.hpp
 *
 *  Created on: 27.01.2015
 *	Author: erfanz
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <stdint.h>


typedef enum {
	LATEST,
	OLDEST,
	RANDOM,
	SKEWED
} version_needed;

typedef enum {
	READ_SHIFT_WRITE,
	FIX_POINTER
} Insert_strategy;


static const uint16_t TCP_PORT 	= 45679;			// the data sites are listening on this port
static const int IB_PORT		= 1;

static const char ITEM_FILENAME[50] = "./data/small_item.data";


static const int MAX_ITEM_CNT		= 2;		// Number of Items
static const int MAX_ORDERS_CNT		= 100100;	// Number of Orders
static const int MAX_CCXACTS_CNT	= 100100;	// Number of CCXacts

static const int MAX_ITEM_VERSIONS		= 1;		// Maximum number of versions per data item
static const int MAX_ORDERS_VERSIONS	= 1;		// Maximum number of versions per data item
static const int MAX_CCXACTS_VERSIONS	= 1;		// Maximum number of versions per data item


static const int TIMEOUT_IN_MS = 500; /* ms */

static const int FETCH_BLOCK_SIZE = 1;		// Number of items fetched at each RDMA READ (only for RDMA mode) 


int TRANSACTIONS_PER_CLIENT 			= 1;

int TRANSACTION_STATEMENT_CNT 			= 10000;

// This parameter sets how many of the first runs should be excluded from the reported statistics
int NUM_OF_EXCLUDING_THE_FIRST_RUNS 	= 5000;	

static const int CLIENTS_CNT 			= 1;

static const double WRITE_RATIO			= -1;

static const version_needed ASSIGNED_READ_TIMESTAMP_TYPE	= SKEWED; 

static const Insert_strategy insert_strategy  = FIX_POINTER;


#endif /* CONFIG_H_ */