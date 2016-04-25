/*
 *	config.hpp
 *
 *  Created on: 27.Jan.2015
 *	Author: erfanz
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <stdint.h>
#include <string>
#include <vector>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


namespace config {
/* Logging			*/
#define DEBUG_ENABLED (false)
#define DEBUG_OUTPUT config::Output::SCREEN
enum Output{FILE, SCREEN};							// Don't change this
static const std::string LOG_FOLDER		= "logs";	// Don't change this, unless you change the Makefile too


/* Server settings */
static const int						SERVER_CNT	= 3;
static const std::vector<std::string>	SERVER_ADDR	= {"192.168.1.1", "192.168.2.1", "192.168.3.1"};
static const std::vector<uint16_t>		TCP_PORT	= {45680, 45680, 45680};
static const std::vector<uint8_t>		IB_PORT		= {1, 1, 1};
static const size_t						SERVER_THREADS_CNT = 40;				// Ideally should be set to the number of CPU on each server machine

/* Oracle settings */
static const std::string	TIMESTAMP_SERVER_ADDR		= "192.168.4.1";	// only relevant for Tranditional-SI
static const uint16_t		TIMESTAMP_SERVER_PORT		= 56788;			// only relevant for Tranditional-SI
static const uint8_t		TIMESTAMP_SERVER_IB_PORT	= 1;				// only relevant for Tranditional-SI


/* Client setting */
static const bool			ADAPTIVE_ABORT_RATE 	= false;
static const double 		MAX_ABORT_RATE			= 0.1;
static const unsigned		ADAPTIVE_WINDOW_SIZE	= 100;


/* Traditional-SI-specific settings */
static const std::string	TRX_MANAGER_ADDR		= "192.168.0.1";	// only relevant for Trad-SI
static const uint16_t		TRX_MANAGER_TCP_PORT	= 45677;			// only relevant for Trad-SI
static const uint8_t		TRX_MANAGER_IB_PORT		= 1;				// only relevant for Trad-SI

static const bool			APPLY_COMMUTATIVE_UPDATES = true;			// the flag for applying commutative updates: those updates which can be implemented using RDMA atomic operations instead of locking.


namespace tpcc_settings{
/* Experiment settings	*/
static const int					TRANSACTION_CNT 		= 100000;					// This is __per client__. For the experiments, we will use 100 000
static const std::vector<double>	TRANSACTION_MIX_RATIOS	= {						// Numbers must add up to 1
		0.45,	// Ratio of New Order
		0.43,	// Ratio of Payment
		0.04,	// Ratio of Order-Status
		0.04,	// Ratio of Delivery
		0.04};	// Ratio of Stock-Level.

/*	Database settings	*/
static const int WAREHOUSE_PER_SERVER		= 1;
static const int WAREHOUSE_CNT				= WAREHOUSE_PER_SERVER * SERVER_CNT;
static const int ITEMS_CNT					= 100000;		// Make sure that this number is >= TPCCUtil::ORDER_MAX_OL_CNT, which is 15 by default. TPCC default is 100000
static const int DISTRICT_PER_WAREHOUSE		= 10; 			// TPCC default is 10;
static const int CUSTOMER_PER_DISTRICT		= 3000; 		// TPCC default is 3000;
static const int STOCK_PER_WAREHOUSE		= ITEMS_CNT;
static const int ORDER_BUFFER_PER_CLIENT	= 1000;			// The buffer size allocated to each client for storing orders/new orders/orderlines. In the ideal case, this should be TRANSACTION_CNT.
static const int HISTORY_BUFFER_PER_CLIENT	= 1000;			// the buffer size allocated to each client for storing history. In the ideal case, this should be TRANSACTION_CNT.
static const double REMOTE_WAREHOUSE_PROB	= 0.01; 		// probability of new order selecting a remote warehouse for ol_supply_w_id. TPCC Default is 0.01
static const int VERSION_NUM 				= 3;			// Number of versions to be kept around for each record, excluding the head version. So the total number of versions will be VERSION_NUM + 1
}	// namespace tpcc_settings


namespace tpcw_settings{
/* Experiment settings */
static const int		TRANSACTION_CNT 		= 1000000;

/* Input data settings */
static const char	ITEM_FILENAME[50]		= "./data/big_item.data";
static const double	SKEWNESS_IN_ITEM_ACCESS	= 0;		// 0 means no skew (random access). 1 is zipf law. Could be arbitrariliy large.
static const int	ITEM_CNT				= 50000 * SERVER_CNT; 	// Number of Items

/* Database settings */
static const int	ITEM_PER_SERVER			= ITEM_CNT / SERVER_CNT;
static const int	ORDERLINE_PER_ORDER		= MIN(SERVER_CNT, 3);
static const int	MAX_ORDERS_CNT			= TRANSACTION_CNT; // TODO TRANSACTION_CNT * CLIENTS_CNT;	// Number of Orders
static const int	MAX_CCXACTS_CNT			= MAX_ORDERS_CNT;	// Number of CCXacts
static const int	MAX_ITEM_VERSIONS	= 3;		// Maximum number of versions per data item
}	// namespace tpcw_settings


}
#endif /* CONFIG_H_ */
