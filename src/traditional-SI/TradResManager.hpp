/*
 *	TradResManager.hpp
 *
 *	Created on: 13.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TRAD_RES_MANAGER_H_
#define TRAD_RES_MANAGER_H_

#include "TradResManagerContext.hpp"
#include "../util/RDMACommon.hpp"
#include "../../config.hpp"
#include "../tpcw-tables/item_version.hpp"
#include "../auxilary/lock.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include <mutex>


class TradResManager {
private:
	static int			res_mng_sockfd;
	static int			tcp_port;
	
	static ItemVersion	*items_region;
	static std::mutex 	item_lock[ITEM_CNT];
	

	static int initialize_data_structures();

	static int destroy_resources ();
	
	static int lock_item(int I_ID);
	
	static int unlock_item(int I_ID);

	static int decremenet_item_stock(int item_id, int quantity, Timestamp* commit_timestamp);
	
	static void* handle_trx_mng_thread(void *param);
	
	
public:
	/******************************************************************************
	* Function: start_server
	*
	* Input
	* server_number (e.g. 0, 1, ...., Config.SERVER_CNT)
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the server. 
	*
	******************************************************************************/
	int start_server (int server_num);
	
	/******************************************************************************
	* Function: usage
	*
	* Input
	* argv0 command line arguments
	*
	* Output
	* none
	*
	* Returns
	* none
	*
	* Description
	* print a description of command line syntax
	******************************************************************************/
	static void usage (const char *argv0);
};
#endif // TRAD_RES_MANAGER_H_ 