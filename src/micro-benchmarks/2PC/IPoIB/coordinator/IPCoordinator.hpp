/*
 *	IPCoordinator.hpp
 *
 *	Created on: 12.Apr.2015
 *	Author: erfanz
 */

#ifndef IP_COORDINATOR_H_
#define IP_COORDINATOR_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../../../util/RDMACommon.hpp"
#include "IPCoordinatorContext.hpp"

class IPCoordinator{
private:
	static int			server_sockfd;		// Server's socket file descriptor
	
	static int start_benchmark(IPCoordinatorContext *ctx);


public:
	
	/******************************************************************************
	* Function: start
	*
	* Input
	* nothing
	*
	* Returns
	* socket (fd) on success, negative error code on failure
	*
	* Description
	* Starts the coordinator. 
	*
	******************************************************************************/
	int start ();
	
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

#endif /* IPCOORDINATOR_H_ */