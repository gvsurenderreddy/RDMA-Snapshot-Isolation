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

typedef benchmark_config::VERB_TYPE_ENUM VERB_TYPE_ENUM;
typedef benchmark_config::MEMORY_ACCESS_ENUM MEM_ACC_ENUM;



class BenchmarkClient{
private:
	uint16_t client_num;
	ClientContext ctx;
	const VERB_TYPE_ENUM verb_type_;
	const MEM_ACC_ENUM memory_access_;

	int perform_operation();
	int start_benchmark();

public:
	BenchmarkClient(VERB_TYPE_ENUM verb_type, MEM_ACC_ENUM memory_access);
	int start_client ();
};

#endif /* BENCHMARK_CLIENT_RDMA_H_ */
