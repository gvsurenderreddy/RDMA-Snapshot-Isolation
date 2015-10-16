/*
 *	IPCohort.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef IP_COHORT_H_
#define IP_COHORT_H_

#include "IPCohortContext.hpp"
#include "../../../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class IPCohort{
private:
	int	server_sockfd;		// Server's socket file descriptor
	int	tcp_port;
		
	static int start_benchmark(IPCohortContext &ctx);
	
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
	int start_server ();
	
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
	
	~IPCohort ();
	
};
#endif /* COHORT_H_ */