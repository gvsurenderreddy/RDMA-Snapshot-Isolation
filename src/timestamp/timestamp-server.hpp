/*
 *	timestamp-server.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef TIMESTAMPSERVER_H_
#define TIMESTAMPSERVER_H_

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include "../../config.hpp"
#include <mutex>

#include "../basic-types/timestamp.hpp"


class TimestampServer{
private:
	static int				server_sockfd;		// Server's socket file descriptor
	
	static Timestamp		timestamp;
	static std::mutex 		timestamp_mutex;
	
	struct TimestampRequest {
		int dumb;
	};
	
	struct TimestampResponse {
		Timestamp commit_timestamp;
	};
	
	struct Context {
		struct ibv_port_attr port_attr;	/* IB port attributes */
		struct ibv_context *ib_ctx;			/* device handle */
		struct ibv_pd *pd;		/* PD handle */
		struct ibv_cq *cq;		/* CQ handle */
		struct ibv_qp *qp;		/* QP handle */
		
		int client_sockfd;		/* TCP socket file descriptor */
		std::string client_ip;
		int	client_port;
		
		int trx_num; 
		
		struct ibv_mr *timestamp_req_mr;
		struct ibv_mr *timestamp_res_mr;
	
		struct TimestampRequest		timestamp_request;
		struct TimestampResponse	timestamp_response;
	};

	static int initialize_data_structures();


	/******************************************************************************
	* Function: create_context
	*
	* Input
	* pointer to the resources structure for the client
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	*
	* This function creates and allocates all necessary system resources. These
	* are stored in res (the input argument).
	*****************************************************************************/
	static int		create_context(struct Context *ctx);

	static void*	handle_client(void *param);
	
	static int		register_memory(struct Context *ctx);

	static int		acquire_commit_timestamp(Timestamp *commit_timestamp);
		
	static std::string	get_full_desc(struct Context *ctx);
	

	/******************************************************************************
	* Function: destroy_resources
	*
	* Input
	* res pointer to resources structure
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	* Cleanup and deallocate all resources used
	******************************************************************************/
	static int destroy_client_context (struct Context *ctx);
	
	static int destroy_resources ();
	
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
#endif /* TIMESTAMPSERVER_H_ */