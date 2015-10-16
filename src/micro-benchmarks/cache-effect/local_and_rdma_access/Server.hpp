/*
 *	Server.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "ServerContext.hpp"
#include "../../../util/RDMACommon.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class Server{
private:
	int	server_sockfd;		// Server's socket file descriptor
	int	tcp_port;
	int	ib_port;
	
	// memory buffers
	struct MyStruct *global_buffer;
	
	
	// *********************************************************************
	// Initializing the array of structs, each element pointing to the next 
	void make_sequential_linked_list(struct MyStruct *myS);
	
	// *********************************************************************
	// Initializing the array of structs, so each element pointing to a random element 
	void make_random_linked_list(struct MyStruct *myS);
	
	int start_benchmark();
	
	int fill_cache();
	
	
	
	
	int initialize_data_structures();
	int initialize_context(ServerContext &ctx);
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
	
	~Server ();
	
};
#endif /* SERVER_H_ */