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

namespace config {
	#define DEBUG_ENABLED (true)

	static const int	SERVER_CNT		= 1;
	//static 		 int	CLIENTS_CNT;			// this is set in run-time


	static const std::string	TRX_MANAGER_ADDR		= "192.168.1.1";	// only relevant for Trad-SI
	static const uint16_t		TRX_MANAGER_TCP_PORT	= 45677;			// only relevant for Trad-SI
	static const uint8_t		TRX_MANAGER_IB_PORT		= 1;				// only relevant for Trad-SI

//	static const std::string	SERVER_ADDR[SERVER_CNT]	= {"192.168.1.1", "192.168.2.1", "192.168.4.1"};
//	static const uint16_t		TCP_PORT[SERVER_CNT]	= {45678, 45678, 45678};
//	static const uint8_t		IB_PORT[SERVER_CNT]		= {1, 1, 1};

	static const std::string	SERVER_ADDR[SERVER_CNT]	= {"192.168.0.1"};
	static const uint16_t		TCP_PORT[SERVER_CNT]	= {45678};
	static const uint8_t		IB_PORT[SERVER_CNT]		= {1};


	static const std::string	TIMESTAMP_SERVER_ADDR		= "192.168.0.1";	// only relevant for Tranditional-SI
	static const uint16_t		TIMESTAMP_SERVER_PORT		= 56789;			// only relevant for Tranditional-SI
	static const int			TIMESTAMP_SERVER_IB_PORT	= 1;	// only relevant for Tranditional-SI
	static const size_t			TIMESTAMP_SERVER_QUEUE_SIZE = 10;	// has to be a multiple of  number of clients
	static const useconds_t		TIMESTAMP_CLEANUP_SLEEP_MICROSEC = 2;

	static const char	ITEM_FILENAME[50]		= "./data/big_item.data";
	static const double	SKEWNESS_IN_ITEM_ACCESS	= 0;		// 0 means no skew (random access). 1 is zipf law. Could be arbitrariliy large.

	static const int	TRANSACTION_CNT 		= 30;//1000000;

	static const int	ITEM_CNT				= 10; //100000;			// Number of Items
	static const int	ITEM_PER_SERVER			= ITEM_CNT / SERVER_CNT;
	static const int	ORDERLINE_PER_ORDER		= SERVER_CNT;		// TODO in current design, it has to be equal to SERVER_CNT

	//static const int	MAX_ORDERS_CNT			= TRANSACTION_CNT * CLIENTS_CNT;	// Number of Orders
	// static const int	MAX_CCXACTS_CNT			= MAX_ORDERS_CNT;	// Number of CCXacts

	static const int	MAX_ITEM_VERSIONS		= 1;		// Maximum number of versions per data item
	static const int	MAX_ORDERS_VERSIONS		= 1;		// Maximum number of versions per data item
	static const int	MAX_ORDERLINE_VERSIONS	= 1;
	static const int	MAX_CCXACTS_VERSIONS	= 1;		// Maximum number of versions per data item

	static const int	TIMEOUT_IN_MS			= 500;		/* ms */

	static const int	FETCH_BLOCK_SIZE		= 1;		// Number of items fetched at each RDMA READ

	// To avoid allocating more memory than is allowed by InfiniBand, we "overwrite" new ORDER_LINEs/CC_XACTS/ORDERS
	// on the old ones, e.g. (MAX_BUFFER_SIZE + 1)'th order_line will be overwritten on the first one
	static const int 	MAX_BUFFER_SIZE 		= 10000;	// TODO

}
#endif /* CONFIG_H_ */
