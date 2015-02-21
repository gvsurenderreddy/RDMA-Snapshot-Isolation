/*
 *	TradTrxManager.hpp
 *
 *	Created on: 9.Feb.2015
 *	Author: erfanz
 */

#ifndef TRAD_TRX_MANAGER_H_
#define TRAD_TRX_MANAGER_H_

#include "TradTrxManagerContext.hpp"
#include "../util/RDMACommon.hpp"
#include "../../config.hpp"
#include "../auxilary/lock.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <mutex>


class TradTrxManager {
private:
	static int			server_sockfd;		// Server's socket file descriptor
	static int			res_mng_socks[SERVER_CNT];
	
	static Timestamp	timestamp;
	static std::mutex 	timestamp_mutex;
	
	// Memory regions
	//OrdersVersion		*orders_region;
	//OrderLineVersion	*order_line_region;
	//CCXactsVersion		*cc_xacts_region;

	
	struct SharedContext {
		struct ibv_context *ib_ctx;	/* device handle */
	};
	
	static struct SharedContext s_ctx;

	int initialize_data_structures();


	int open_device(struct ibv_context** ib_ctx);
	
	int build_connection(int ib_port, struct ibv_context* ib_ctx,
	struct ibv_port_attr* port_attr, struct ibv_pd **pd, struct ibv_cq **cq, int cq_size);
	
	
	static void*	handle_client(void *param);

	static int		acquire_commit_timestamp(Timestamp *commit_timestamp);
	
	static int		start_transactions(TradTrxManagerContext &ctx);
	
	static std::string get_full_desc(BaseContext &ctx);
	
	int destroy_resources ();
	
public:
	
	/******************************************************************************
	* Function: start_server
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the server. 
	*
	******************************************************************************/
	int start_server ();
	
	static void usage (const char *argv0);
};
#endif // TRADITIONAL_TRX_MANAGER_H_