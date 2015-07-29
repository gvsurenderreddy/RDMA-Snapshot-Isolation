/*
 *	IPCohort.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "IPCohort.hpp"
#include "../../../../../config.hpp"
#include "../../../benchmark-config.hpp"
#include "../../../../util/utils.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <sys/resource.h>	// getrusage()


IPCohort::~IPCohort () {
	close(server_sockfd);
}

void IPCohort::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << std::endl;
	std::cout << "starts a IPCohort server and connects it to the coordinator on Config.TRX_MANAGER_ADDR" << std::endl;
}

int IPCohort::start_benchmark(IPCohortContext &ctx) {
	char temp_char;
	unsigned long long cpu_checkpoint_start, cpu_checkpoint_finish;
	int iteration = 0;
	struct rusage usage;
    struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;
	
	
	DEBUG_COUT("[in handle client]");
	
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	
	DEBUG_COUT("[Synced with client]");
	
    // For CPU usage in terms of time
	getrusage(RUSAGE_SELF, &usage);
    start_kernel_usage = usage.ru_stime;
    start_user_usage = usage.ru_utime;
	
	
	// For CPU usage in terms of clocks (ticks)
	cpu_checkpoint_start = rdtsc();
	
	while (iteration < OPERATIONS_CNT) {
		//TEST_NZ (RDMACommon::poll_completion(ctx.cq));	// for Receive
		TEST_NZ (sock_read(ctx.sockfd, (char *)&(ctx.recv_data_msg), sizeof(int)));	
		DEBUG_COUT("[READ] --READY-- request from coordinator");
		
		
		//TEST_NZ (RDMACommon::post_SEND(ctx.qp, ctx.send_data_mr, (uintptr_t)&ctx.send_data_msg, sizeof(int), false));
		TEST_NZ (sock_write(ctx.sockfd, (char *)&(ctx.send_data_msg), sizeof(int)));			
		DEBUG_COUT("[WRIT] --YES-- response to coordinator (without completion)");
		
		
		//TEST_NZ (RDMACommon::poll_completion(ctx.cq));	// for Receive
		TEST_NZ (sock_read(ctx.sockfd, (char *)&(ctx.recv_data_msg), sizeof(int)));	
		DEBUG_COUT("[READ] --COMMIT-- request from coordinator");
		
		//TEST_NZ (RDMACommon::post_SEND(ctx.qp, ctx.send_data_mr, (uintptr_t)&ctx.send_data_msg, sizeof(int), true));
		TEST_NZ (sock_write(ctx.sockfd, (char *)&(ctx.send_data_msg), sizeof(int)));			
		DEBUG_COUT("[WRIT] --DONE-- response to coordinator");
		
		iteration++;
	}
	cpu_checkpoint_finish = rdtsc();
	
    getrusage(RUSAGE_SELF, &usage);
    end_user_usage = usage.ru_utime;
    end_kernel_usage = usage.ru_stime;
	
	
	double user_cpu_microtime = ( end_user_usage.tv_sec - start_user_usage.tv_sec ) * 1E6 + ( end_user_usage.tv_usec - start_user_usage.tv_usec );
	double kernel_cpu_microtime = ( end_kernel_usage.tv_sec - start_kernel_usage.tv_sec ) * 1E6 + ( end_kernel_usage.tv_usec - start_kernel_usage.tv_usec );
	
	
	DEBUG_COUT("[Info] Cohort Client Done");
	
	unsigned long long average_cpu_clocks = (cpu_checkpoint_finish - cpu_checkpoint_start) / OPERATIONS_CNT;
	
	std::cout << "[Stat] AVG total CPU elapsed time (u sec):    	" << (user_cpu_microtime + kernel_cpu_microtime) / OPERATIONS_CNT << std::endl;
	std::cout << "[Stat] Average CPU clocks:    	" << average_cpu_clocks << std::endl;
	return 0;
}


int IPCohort::start_server () {	
	IPCohortContext ctx;
	char temp_char;
		
	TEST_NZ (establish_tcp_connection(TRX_MANAGER_ADDR.c_str(), TRX_MANAGER_TCP_PORT, &ctx.sockfd));
	
	TEST_NZ (ctx.create_context());
	
	srand (time(NULL));		// initialize random seed
	
	start_benchmark(ctx);
	
	/* Sync so server will know that client is done mucking with its memory */
	DEBUG_COUT("[Info] Cohort client is done, and is ready to destroy its resources!");
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	TEST_NZ(ctx.destroy_context());
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		IPCohort::usage(argv[0]);
		return 1;
	}
	
	pin_to_CPU (0);	// pin the current process to CPU core 0
	
	IPCohort cohort;
	cohort.start_server();
	return 0;
}