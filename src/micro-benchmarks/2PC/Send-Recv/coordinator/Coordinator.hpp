/*
 *	Coordinator.hpp
 *
 *	Created on: 12.Apr.2015
 *	Author: Erfan Zamanian
 */

#ifndef COORDINATOR_H_
#define COORDINATOR_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#include "../../../../util/RDMACommon.hpp"
#include "CoordinatorContext.hpp"

class Coordinator{
private:
	static int			server_sockfd;		// Server's socket file descriptor
	
	static int start_benchmark(CoordinatorContext *ctx);


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

#endif /* COORDINATOR_H_ */