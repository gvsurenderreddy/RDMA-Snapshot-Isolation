/*
 *	BenchmarkClientRDMA.hpp
 *
 *	Created on: 26.March.2015
 *	Author: erfanz
 */

#ifndef BENCHMARK_CLIENT_RDMA_H_
#define BENCHMARK_CLIENT_RDMA_H_

#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#include "../../util/RDMACommon.hpp"
#include "ClientContext.hpp"

class BenchmarkClientRDMA{
private:
	static int start_benchmark(ClientContext *ctx);
	static int acquire_commit_timestamp(ClientContext &ctx);

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

#endif /* BENCHMARK_CLIENT_RDMA_H_ */