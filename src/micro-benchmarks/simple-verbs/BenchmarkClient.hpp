/*
 *	BenchmarkClient.hpp
 *
 *	Created on: 26.March.2015
 *	Author: Erfan Zamanian
 */

#ifndef BENCHMARK_CLIENT_RDMA_H_
#define BENCHMARK_CLIENT_RDMA_H_

#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

#include "../../util/RDMACommon.hpp"
#include "ClientContext.hpp"

class BenchmarkClient{
private:
	uint16_t client_num;
	ClientContext ctx;
	int perform_operation();
	int start_benchmark();

public:
	BenchmarkClient();
	int start_client ();
};

#endif /* BENCHMARK_CLIENT_RDMA_H_ */
