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
#define MIN(a, b) (((a) < (b)) ? (a) : (b))



namespace config {
	#define DEBUG_ENABLED (false)

	static const int	SERVER_CNT		= 1;

	static const std::string	TRX_MANAGER_ADDR		= "192.168.0.1";	// only relevant for Trad-SI
	static const uint16_t		TRX_MANAGER_TCP_PORT	= 45677;			// only relevant for Trad-SI
	static const uint8_t		TRX_MANAGER_IB_PORT		= 1;				// only relevant for Trad-SI

	static const std::string	SERVER_ADDR[SERVER_CNT]	= {"192.168.1.1"};
	static const uint16_t		TCP_PORT[SERVER_CNT]	= {45678};
	static const uint8_t		IB_PORT[SERVER_CNT]		= {1};

	static const std::string	TIMESTAMP_SERVER_ADDR		= "192.168.0.1";	// only relevant for Tranditional-SI
	static const uint16_t		TIMESTAMP_SERVER_PORT		= 56789;			// only relevant for Tranditional-SI
	static const int			TIMESTAMP_SERVER_IB_PORT	= 1;	// only relevant for Tranditional-SI
	static const size_t			TIMESTAMP_SERVER_QUEUE_SIZE = 60000;	// has to be a multiple of  number of clients
	static const useconds_t		TIMESTAMP_CLEANUP_SLEEP_MICROSEC = 2;

	static const char	ITEM_FILENAME[50]		= "./data/big_item.data";
	static const double	SKEWNESS_IN_ITEM_ACCESS	= 0;		// 0 means no skew (random access). 1 is zipf law. Could be arbitrariliy large.

	static const int	TRANSACTION_CNT 		= 1000000;
	static const int	ITEM_CNT				= 50000 * SERVER_CNT; 	// Number of Items
	static const int	ITEM_PER_SERVER			= ITEM_CNT / SERVER_CNT;
	static const int	ORDERLINE_PER_ORDER		= MIN(SERVER_CNT, 3);

	static const int	MAX_ORDERS_CNT			= TRANSACTION_CNT; // TODO TRANSACTION_CNT * CLIENTS_CNT;	// Number of Orders
	static const int	MAX_CCXACTS_CNT			= MAX_ORDERS_CNT;	// Number of CCXacts

	static const uint16_t	MAX_ITEM_VERSIONS	= 3;		// Maximum number of versions per data item
	//static const int	MAX_ORDERS_VERSIONS		= 1;		// Maximum number of versions per data item
	//static const int	MAX_ORDERLINE_VERSIONS	= 1;
	//static const int	MAX_CCXACTS_VERSIONS	= 1;		// Maximum number of versions per data item

	static const int	TIMEOUT_IN_MS			= 500;		/* ms */
}
#endif /* CONFIG_H_ */
