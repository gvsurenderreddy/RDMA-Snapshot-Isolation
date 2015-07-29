/*
 *	TradClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#ifndef TRAD_CLIENT_H_
#define TRAD_CLIENT_H_

#include "TradClientContext.hpp"
#include "../../config.hpp"
#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include <vector>



class TradClient{
private:

	static void fill_shopping_cart(struct Cart *cart);
	
	int start_transaction(TradClientContext &ctx);	
	
	static std::vector<int> preselected_item_ids[4];
	

public:
	/******************************************************************************
	* Function: start_client
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the client. 
	*
	******************************************************************************/
	int start_client ();
	
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

#endif /* TRAD_CLIENT_H_ */
