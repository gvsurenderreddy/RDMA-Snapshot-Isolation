/*
 *	TradResManager.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef BASE_TRAD_RES_MANAGER_H_
#define BASE_TRAD_RES_MANAGER_H_

#include "../../config.hpp"
#include "../auxilary/lock.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include <mutex>

#include "../two-sided/TradMessage.hpp"


class BaseTradResManager {
protected:
	static int			res_mng_sockfd;
	static int			tcp_port;
	
	static ItemVersion	*items_region;
	static std::mutex 	item_lock[ITEM_CNT];
	

	static int initialize_data_structures();

	static int destroy_resources ();
	
	static int lock_item(int I_ID);
	
	static int unlock_item(int I_ID);

	static int decremenet_item_stock(int item_id, int quantity, Timestamp* commit_timestamp);
		
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
	virtual int start_server (int server_num) = 0;

public:
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
#endif // BASE_TRAD_RES_MANAGER_H_ 