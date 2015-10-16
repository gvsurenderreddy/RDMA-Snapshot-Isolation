/*
 *	BaseTradTrxManager.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef BASE_TRAD_TRX_MANAGER_H_
#define BASE_TRAD_TRX_MANAGER_H_

#include "../util/BaseContext.hpp"	// TODO: this is only for get_full_desc
#include "../../config.hpp"
#include "TradMessage.hpp"
#include "../auxilary/lock.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <mutex>


class BaseTradTrxManager {
protected:
	static int			server_sockfd;		// Server's socket file descriptor
	static int			res_mng_socks[SERVER_CNT];
	
	static Timestamp	timestamp;
	static std::mutex 	timestamp_mutex;
	
	// Memory regions
	//OrdersVersion		*orders_region;
	//OrderLineVersion	*order_line_region;
	//CCXactsVersion	*cc_xacts_region;

	int 		initialize_data_structures();

	static int	acquire_commit_timestamp(Timestamp *commit_timestamp);
		
	static std::string get_full_desc(BaseContext &ctx);
	
	int destroy_resources ();
	
public:
	static void usage (const char *argv0);
};
#endif // BASE_TRADITIONAL_TRX_MANAGER_H_