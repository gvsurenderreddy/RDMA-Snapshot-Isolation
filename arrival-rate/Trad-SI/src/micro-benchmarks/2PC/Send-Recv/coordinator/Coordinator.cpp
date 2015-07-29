/*
 *	Coordinator.cpp
 *
 *	Created on: 12.Apr.2015
 *	Author: erfanz
 */

#include "Coordinator.hpp"
#include "../../../../util/utils.hpp"
#include "../../../../../config.hpp"
#include "../../../benchmark-config.hpp"


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <time.h>	// for struct timespec
#include <sys/resource.h>	// getrusage()


int	Coordinator::server_sockfd	= -1;		// Server's socket file descriptor


void Coordinator::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

int Coordinator::start_benchmark(CoordinatorContext *ctx) {
	int signaledPosts = 0;
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec beforeSending, afterSending;				// for calculating TPMS
	
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock;
    
	struct rusage usage;
    struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;
	char temp_char;
	
	unsigned long long cpu_checkpoint_start, cpu_checkpoint_2, cpu_checkpoint_3, cpu_checkpoint_finish ;
	bool signalled_flag = false;	
	
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	}
	
	
	DEBUG_COUT ("[Info] Benchmark now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
    getrusage(RUSAGE_SELF, &usage);
    start_kernel_usage = usage.ru_stime;
    start_user_usage = usage.ru_utime;
	
	int iteration = 0;
	
	cpu_checkpoint_start = rdtsc();
	while (iteration < OPERATIONS_CNT) {
		DEBUG_COUT("iteration " << iteration);

		// Sent: Ready??
		for (int i = 0; i < SERVER_CNT; i++) {
			TEST_NZ (RDMACommon::post_RECEIVE(ctx[i].qp, ctx[i].recv_data_mr, (uintptr_t)&ctx[i].recv_data_msg, sizeof(int)));
			DEBUG_COUT("[Info] Receive posted");
			
			TEST_NZ (RDMACommon::post_SEND(ctx[i].qp, ctx[i].send_data_mr, (uintptr_t)&ctx[i].send_data_msg, sizeof(int), false));
			DEBUG_COUT("[Sent] --READY-- request sent to cohort " << i);
			
		}
		
		// Received: Ready!
		for (int i = 0; i < SERVER_CNT; i++) {
			TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));	// for RECV
			//TEST_NZ (RDMACommon::event_based_poll_completion(ctx[i].comp_channel, ctx[i].cq)); // for RECV
			DEBUG_COUT("[Recv] --YES-- response received from cohort " << i);
		}
		
		// Sent: Commit
		for (int i = 0; i < SERVER_CNT; i++) {
			TEST_NZ (RDMACommon::post_RECEIVE(ctx[i].qp, ctx[i].recv_data_mr, (uintptr_t)&ctx[i].recv_data_msg, sizeof(int)));
			DEBUG_COUT("[Info] Receive posted");
			
			
			if (iteration % 500 == 0) {
				signalled_flag = true;
				TEST_NZ (RDMACommon::post_SEND(ctx[i].qp, ctx[i].send_data_mr, (uintptr_t)&ctx[i].send_data_msg, sizeof(int), true));
				DEBUG_COUT("[Sent] --COMMIT-- request sent to cohort " << i);
			}
			else{
				signalled_flag = false;
				TEST_NZ (RDMACommon::post_SEND(ctx[i].qp, ctx[i].send_data_mr, (uintptr_t)&ctx[i].send_data_msg, sizeof(int), false));
				DEBUG_COUT("[Sent] --COMMIT-- request sent to cohort (unsignalled) " << i);
			}
		}
		
		// collect completion events for SEND
		if (signalled_flag){
			for (int i = 0; i < SERVER_CNT; i++) {
				TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));	// for SENT
				//TEST_NZ (RDMACommon::event_based_poll_completion(ctx[i].comp_channel, ctx[i].cq));	// for SENT
			}
		}
		
		// Received: Done!
		for (int i = 0; i < SERVER_CNT; i++) {
			TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));	// for RECV
			//TEST_NZ (RDMACommon::event_based_poll_completion(ctx[i].comp_channel, ctx[i].cq)); // for RECV
		
			
			DEBUG_COUT("[Recv] --DONE-- response received from cohort " << i);
		}
		
		iteration++;
	}
	cpu_checkpoint_finish = rdtsc();

    getrusage(RUSAGE_SELF, &usage);
    end_user_usage = usage.ru_utime;
    end_kernel_usage = usage.ru_stime;
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	double user_cpu_microtime = ( end_user_usage.tv_sec - start_user_usage.tv_sec ) * 1E6 + ( end_user_usage.tv_usec - start_user_usage.tv_usec );
	double kernel_cpu_microtime = ( end_kernel_usage.tv_sec - start_kernel_usage.tv_sec ) * 1E6 + ( end_kernel_usage.tv_usec - start_kernel_usage.tv_usec );
	
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	
	double latency_in_micro = (double)(micro_elapsed_time / OPERATIONS_CNT);
	//double latency_in_micro = (double)(cumulative_latency / signaledPosts) / 1000;
	
	double mega_byte_per_sec = ((sizeof(int) * OPERATIONS_CNT / 1E6 ) / (micro_elapsed_time / 1E6) );
	double operations_per_sec = OPERATIONS_CNT / (micro_elapsed_time / 1E6);
	double cpu_utilization = (user_cpu_microtime + kernel_cpu_microtime) / micro_elapsed_time;
	
	unsigned long long average_cpu_clocks = (cpu_checkpoint_finish - cpu_checkpoint_start) / OPERATIONS_CNT;
	
	std::cout << "[Stat] Avg latency(u sec):   	" << latency_in_micro << std::endl; 
	std::cout << "[Stat] MegaByte per Sec:   	" << mega_byte_per_sec <<  std::endl;
	std::cout << "[Stat] Operations per Sec:   	" << operations_per_sec <<  std::endl;
	std::cout << "[Stat] CPU utilization:    	" << cpu_utilization << std::endl;
	std::cout << "[Stat] USER CPU utilization:    	" << user_cpu_microtime / micro_elapsed_time << std::endl;
	std::cout << "[Stat] KERNEL CPU utilization:    	" << kernel_cpu_microtime / micro_elapsed_time << std::endl;
	std::cout << "[Stat] AVG USER CPU elapsed time (u sec):    	" << user_cpu_microtime / OPERATIONS_CNT << std::endl;
	std::cout << "[Stat] AVG KERNEL CPU elapsed time (u sec):    	" << kernel_cpu_microtime / OPERATIONS_CNT << std::endl;
	std::cout << "[Stat] AVG total CPU elapsed time (u sec):    	" << (user_cpu_microtime + kernel_cpu_microtime) / OPERATIONS_CNT << std::endl;
	
	std::cout << "[Stat] Average CPU clocks:    	" << average_cpu_clocks << std::endl;
	
	
	std::cout  << latency_in_micro << '\t' << mega_byte_per_sec << '\t' << operations_per_sec << '\t' << cpu_utilization << std::endl;
	
	
	return 0;
}



int Coordinator::start () {	
	CoordinatorContext ctx[SERVER_CNT];
	
	struct sockaddr_in returned_addr;
	socklen_t len = sizeof(returned_addr);
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed
	
	
	// Call socket(), bind() and listen()
	TEST_NZ (server_socket_setup(&server_sockfd, SERVER_CNT));
	
	// accept connections from Cohorts
	std::cout << "[Info] Waiting for " << SERVER_CNT << " cohort(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;
	for (int s = 0; s < SERVER_CNT; s++) {
		ctx[s].sockfd  = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[s].sockfd < 0){ 
			std::cerr << "ERROR on accept() for RM #" << s << std::endl;
			return -1;
		}
		
		std::cout << "[Conn] Received Cohort #" << s << " on sock " << ctx[s].sockfd << std::endl;
		// create all resources
	
		ctx[s].ib_port = TRX_MANAGER_IB_PORT;
		TEST_NZ (ctx[s].create_context());
		DEBUG_COUT("[Info] Context for cohort " << s << " created");
		
		
		// connect the QPs
		TEST_NZ (RDMACommon::connect_qp (&(ctx[s].qp), ctx[s].ib_port, ctx[s].port_attr.lid, ctx[s].sockfd));	
		DEBUG_COUT("[Conn] QPed to cohort " << s);
	}
	std::cout << "[Info] Established connection to all " << SERVER_CNT << " resource-manager(s)." << std::endl; 
	
	
	start_benchmark(ctx);
	
	DEBUG_COUT("[Info] Coordinator is done, and is ready to destroy its resources!");
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT("[Conn] Notified cohort " << i << " it's done");
		TEST_NZ ( ctx[i].destroy_context());
	}
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		Coordinator::usage(argv[0]);
		return 1;
	}
	
	pin_to_CPU (0);	// pin the current process to CPU core 0
	
	Coordinator coordinator;
	coordinator.start();
	return 0;
}