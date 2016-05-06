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
enum 	Output{FILE, SCREEN};						// Don't change this. Possible options for logging.
#define DEBUG_OUTPUT config::Output::SCREEN			// Where to write the logs (possible options are specified in Output enum.
static const std::string LOG_FOLDER		= "logs";	// Don't change this, unless you change the Makefile too


/* Server settings */
static const size_t						SERVER_CNT	= 3;
static const std::vector<std::string>	SERVER_ADDR = {"192.168.1.1", "192.168.2.1", "192.168.3.1"};		// IP address of the servers
static const std::vector<uint16_t>		TCP_PORT	= {45680, 45680, 45680};							// TCP port of the servers
static const std::vector<uint8_t>		IB_PORT		= {1, 1, 1};										// InfiniBand port of the servers
static const size_t						SERVER_THREADS_CNT = 40;										// Number of threads running on each server for handling index requests. Ideally should be set to the number of CPU on each server machine

/* Oracle settings */
static const std::string	TIMESTAMP_SERVER_ADDR		= "192.168.0.1";						// IP address of the oracle
static const uint16_t		TIMESTAMP_SERVER_PORT		= 56788;								// TCP port of the oracle
static const uint8_t		TIMESTAMP_SERVER_IB_PORT	= 1;									// IB port of the oracle
enum 						SnapshotAcquisitionType{COMPLETE, ONLY_READ_SET};					// Don't change this. Possible options for snapshot acquisition.
static const SnapshotAcquisitionType	SNAPSHOT_ACQUISITION_TYPE = SnapshotAcquisitionType::ONLY_READ_SET;


/* Client setting */
static const bool			ADAPTIVE_ABORT_RATE 		= false;
static const double 		MAX_ABORT_RATE				= 0.1;
static const unsigned		ADAPTIVE_WINDOW_SIZE		= 100;
static const bool			APPLY_COMMUTATIVE_UPDATES 	= true;		// whether or not commutative updates should be applied for record updates which can be implemented using RDMA atomic operations instead of locking.
static const bool			LOCALITY_EXPLOITAION 		= false;	// Whether or not co-located servers and clients could exchange data through memcpy instead of over the wire

namespace recovery_settings {
static const bool		RECOVERY_ENABLED		= true;				// whether or not logging should be enabled
static const size_t 	LOG_REPLICATION_DEGREE	= MIN(SERVER_CNT, 2);	// how many machines the logs should be written to
static const size_t 	ENTRY_PER_LOG_JOURNAL 	= 100;					// the size of each client's log
static const size_t 	COMMAND_LOG_SIZE 		= 200;					// the maximum size of command in bytes
}

namespace tpcc_settings{
/* Experiment settings	*/
static const unsigned				TRANSACTION_CNT 		= 100000;		// This is __per client__. For the experiments, we will use 1,000,000
static const std::vector<double>	TRANSACTION_MIX_RATIOS	= {			// Numbers must add up to 1
		0.45,	// Ratio of New Order
		0.43,	// Ratio of Payment
		0.04,	// Ratio of Order-Status
		0.04,	// Ratio of Delivery
		0.04};	// Ratio of Stock-Level.

/*	Database settings	*/
static const size_t WAREHOUSE_PER_SERVER		= 1;			// Number of warehouses per server
static const size_t WAREHOUSE_CNT				= WAREHOUSE_PER_SERVER * SERVER_CNT;
static const size_t ITEMS_CNT					= 100000;		// Make sure that this number is >= TPCCUtil::ORDER_MAX_OL_CNT, which is 15 by default. TPCC default is 100000
static const size_t DISTRICT_PER_WAREHOUSE		= 10; 			// TPCC default is 10;
static const size_t CUSTOMER_PER_DISTRICT		= 3000; 		// TPCC default is 3000;
static const size_t STOCK_PER_WAREHOUSE			= ITEMS_CNT;
static const size_t ORDER_BUFFER_PER_CLIENT		= 1000;			// The buffer size allocated to each client for storing orders/new orders/orderlines. In the ideal case, this should be TRANSACTION_CNT.
static const size_t HISTORY_BUFFER_PER_CLIENT	= 1000;			// the buffer size allocated to each client for storing history. In the ideal case, this should be TRANSACTION_CNT.
static const double REMOTE_WAREHOUSE_PROB		= 0.01; 		// probability of new order selecting a remote warehouse for ol_supply_w_id. TPCC Default is 0.01
static const size_t VERSION_NUM 				= 3;			// Number of versions to be kept around for each record, excluding the head version. So the total number of versions will be VERSION_NUM + 1
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
static const int	MAX_ITEM_VERSIONS		= 3;		// Maximum number of versions per data item
}	// namespace tpcw_settings


}
#endif /* CONFIG_H_ */
