/*
 *	IPCoordinator.cpp
 *
 *	Created on: 12.Apr.2015
 *	Author: Erfan Zamanian
 */

#include "IPCoordinator.hpp"
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
#include <netinet/tcp.h>	// for setsockopt
#include <time.h>	// for struct timespec
#include <sys/resource.h>	// getrusage()



int	IPCoordinator::server_sockfd	= -1;		// Server's socket file descriptor


void IPCoordinator::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

int IPCoordinator::start_benchmark(IPCoordinatorContext *ctx) {
	int signaledPosts = 0;
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec beforeSending, afterSending;				// for calculating TPMS
	
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock;
    
	struct rusage usage;
    struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;
	char temp_char;
	
	unsigned long long cpu_checkpoint_start, cpu_checkpoint_2, cpu_checkpoint_3, cpu_checkpoint_finish ;
	
	
	
	// struct timeval A_user, B_user, C_user, D_user, E_user, A_kernel, B_kernel, C_kernel, D_kernel, E_kernel;
	// double total_AB_user = 0, total_BC_user = 0, total_CD_user = 0, total_DE_user = 0;
	// double total_AB_kernel = 0, total_BC_kernel = 0, total_CD_kernel = 0, total_DE_kernel = 0;
	
	
	
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	}
	
	
	DEBUG_COUT ("[Info] Benchmark now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
    getrusage(RUSAGE_SELF, &usage);
    start_kernel_usage = usage.ru_stime;
    start_user_usage = usage.ru_utime;
	
	int iteration = 0;
	
	
	
	unsigned long long A =0 , B = 0, C = 0;
	
	cpu_checkpoint_start = rdtsc();
	while (iteration < OPERATIONS_CNT) {
		DEBUG_COUT("iteration " << iteration);



	    // getrusage(RUSAGE_SELF, &usage);
	    // A_kernel = usage.ru_stime;
	    // A_user = usage.ru_utime;
		
		
		// Sent: Ready??
		for (int i = 0; i < SERVER_CNT; i++) {			
			//TEST_NZ (RDMACommon::post_SEND(ctx[i].qp, ctx[i].send_data_mr, (uintptr_t)&ctx[i].send_data_mr, sizeof(int), false));
			TEST_NZ (sock_write(ctx[i].sockfd, (char *)&(ctx[i].send_data_msg), sizeof(int)));			
			DEBUG_COUT("[WRIT] --READY-- request sent to cohort " << i);
		}
		
	    // getrusage(RUSAGE_SELF, &usage);
	    // B_kernel = usage.ru_stime;
	    // B_user = usage.ru_utime;
		
		
		
		// Received: Ready!
		for (int i = 0; i < SERVER_CNT; i++) {
			//TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));	// for RECV
			TEST_NZ (sock_read(ctx[i].sockfd, (char *)&(ctx[i].recv_data_msg), sizeof(int)));	
			DEBUG_COUT("[READ] --YES-- response received from cohort " << i);
		}
		
	    // getrusage(RUSAGE_SELF, &usage);
	    // C_kernel = usage.ru_stime;
	    // C_user = usage.ru_utime;
		
		// Sent: Commit
		for (int i = 0; i < SERVER_CNT; i++) {
			//TEST_NZ (RDMACommon::post_SEND(ctx[i].qp, ctx[i].send_data_mr, (uintptr_t)&ctx[i].send_data_msg, sizeof(int), true));
			TEST_NZ (sock_write(ctx[i].sockfd, (char *)&(ctx[i].send_data_msg), sizeof(int)));			
				
			DEBUG_COUT("[WRIT] --COMMIT-- request sent to cohort " << i);
		}
		
	    // getrusage(RUSAGE_SELF, &usage);
	    // D_kernel = usage.ru_stime;
	    // D_user = usage.ru_utime;
		
		// Received: Done!
		for (int i = 0; i < SERVER_CNT; i++) {
			//TEST_NZ (RDMACommon::poll_completion(ctx[i].cq));	// for RECV
			TEST_NZ (sock_read(ctx[i].sockfd, (char *)&(ctx[i].recv_data_msg), sizeof(int)));	
			
			DEBUG_COUT("[READ] --DONE-- response received from cohort " << i);
		}
		
		// getrusage(RUSAGE_SELF, &usage);
		// E_kernel = usage.ru_stime;
		// E_user = usage.ru_utime;
		
		
		// total_AB_user += ( B_user.tv_sec - A_user.tv_sec ) * 1E6 + ( B_user.tv_usec - A_user.tv_usec );
		// total_AB_kernel += ( B_kernel.tv_sec - A_kernel.tv_sec ) * 1E6 + ( B_kernel.tv_usec - A_kernel.tv_usec );
		//
		// total_BC_user += ( C_user.tv_sec - B_user.tv_sec ) * 1E6 + ( C_user.tv_usec - B_user.tv_usec );
		// total_BC_kernel += ( C_kernel.tv_sec - B_kernel.tv_sec ) * 1E6 + ( C_kernel.tv_usec - B_kernel.tv_usec );
		//
		// total_CD_user += ( D_user.tv_sec - C_user.tv_sec ) * 1E6 + ( D_user.tv_usec - C_user.tv_usec );
		// total_CD_kernel += ( D_kernel.tv_sec - C_kernel.tv_sec ) * 1E6 + ( D_kernel.tv_usec - C_kernel.tv_usec );
		//
		// total_DE_user += ( E_user.tv_sec - D_user.tv_sec ) * 1E6 + ( E_user.tv_usec - D_user.tv_usec );
		// total_DE_kernel += ( E_kernel.tv_sec - D_kernel.tv_sec ) * 1E6 + ( E_kernel.tv_usec - D_kernel.tv_usec );
		
		
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
	
	
	// std::cout << "[Stat] AVG A - B (u sec):    	" << (total_AB_user + total_AB_kernel) / OPERATIONS_CNT << std::endl;
	// std::cout << "[Stat] AVG B - C (u sec):    	" << (total_BC_user + total_BC_kernel) / OPERATIONS_CNT << std::endl;
	// std::cout << "[Stat] AVG C - D (u sec):    	" << (total_CD_user + total_CD_kernel) / OPERATIONS_CNT << std::endl;
	// std::cout << "[Stat] AVG D - E (u sec):    	" << (total_DE_user + total_DE_kernel) / OPERATIONS_CNT << std::endl;
	
	
	
	
	
	return 0;
}



int IPCoordinator::start () {
	IPCoordinatorContext ctx[SERVER_CNT];
	
	struct sockaddr_in serv_addr, returned_addr;
	socklen_t len = sizeof(returned_addr);
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed
	
	// Open Socket
	server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		std::cerr << "Error opening socket" << std::endl;
		return -1;
	}
	
	int flag = 1;
	int opt_return = setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if (opt_return < 0) {
		std::cerr << "Could not set sock options" << std::endl;
		return -1;
	}
	
	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TRX_MANAGER_TCP_PORT);
	TEST_NZ(bind(server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	
	// listen				  
	TEST_NZ(listen (server_sockfd, SERVER_CNT));

	
	// accept connections from Cohorts
	std::cout << "[Info] IPCoordinator Waiting for " << SERVER_CNT << " cohort(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;
	for (int s = 0; s < SERVER_CNT; s++) {
		ctx[s].sockfd  = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[s].sockfd < 0){ 
			std::cerr << "ERROR on accept() for RM #" << s << std::endl;
			return -1;
		}
		
		std::cout << "[Conn] Received Cohort #" << s << " on sock " << ctx[s].sockfd << std::endl;
		// create all resources
	}
	std::cout << "[Info] Established connection to all " << SERVER_CNT << " cohort(s)." << std::endl; 
	
	
	start_benchmark(ctx);
	
	DEBUG_COUT("[Info] IPCoordinator is done, and is ready to destroy its resources!");
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT("[Conn] Notified cohort " << i << " it's done");
		TEST_NZ ( ctx[i].destroy_context());
	}
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		IPCoordinator::usage(argv[0]);
		return 1;
	}
	
	pin_to_CPU (0);	// pin the current process to CPU core 0
		
	IPCoordinator coordinator;
	coordinator.start();
	return 0;
}