/*
 *	BenchmarkClient.cpp
 *
 *	Created on: 26.March.2015
 *	Author: erfanz
 */

#include "BenchmarkClient.hpp"
#include "../../util/utils.hpp"
#include "../benchmark-config.hpp"
#include "../../../config.hpp"

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


void BenchmarkClient::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

/******************** For Read
int BenchmarkClient::start_benchmark(ClientContext *ctx) {
	int		server_num;
	int		abort_cnt = 0;
	bool	abort_flag;
	double cumulative_latency = 0;
	int signaledPosts = 0;
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec beforeSending, afterSending;				// for calculating TPMS
	
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock;
    
	struct rusage usage;
    struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;
	char temp_char;
	
	
	TEST_NZ (sock_sync_data (ctx->sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	
	
	DEBUG_COUT ("[Info] Benchmark now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
    getrusage(RUSAGE_SELF, &usage);
    start_kernel_usage = usage.ru_stime;
    start_user_usage = usage.ru_utime;
	
	int offset;
	uint64_t *remote_address;
	
	int iteration = 0;
	while (iteration < OPERATIONS_CNT) {
		offset = rand() % (SERVER_REGION_SIZE - BUFFER_SIZE);
		remote_address = (uint64_t *)(offset + ((uint64_t)ctx->peer_data_mr.addr));
		
		if (iteration % 1000 == 0) {
			//clock_gettime(CLOCK_REALTIME, &beforeSending);	// Fire the  timer
			
			TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			ctx->qp,
			ctx->recv_data_mr,
			(uint64_t)ctx->recv_data_msg,
			&(ctx->peer_data_mr),
			(uint64_t)remote_address,
			(uint32_t)BUFFER_SIZE,
			true));
			
			TEST_NZ (RDMACommon::poll_completion(ctx->cq));
			//TEST_NZ (RDMACommon::event_based_poll_completion(ctx->comp_channel, ctx->cq));
			
			//clock_gettime(CLOCK_REALTIME, &afterSending);	// Fire the  timer
			
			//cumulative_latency =+ ( ( afterSending.tv_sec - beforeSending.tv_sec ) * 1E9 + ( afterSending.tv_nsec - beforeSending.tv_nsec ) );
			//signaledPosts++;
		}
		else {
			TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			ctx->qp,
			ctx->recv_data_mr,
			(uint64_t)ctx->recv_data_msg,
			&(ctx->peer_data_mr),
			(uint64_t)remote_address,
			(uint32_t)BUFFER_SIZE,
			false));
		}
	
		//DEBUG_COUT("[RDMA] Response sent");
		//DEBUG_COUT("[Info] Buffer value: " << ctx->local_buffer);
		iteration++;
	}

    getrusage(RUSAGE_SELF, &usage);
    end_user_usage = usage.ru_utime;
    end_kernel_usage = usage.ru_stime;
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	double user_cpu_microtime = ( end_user_usage.tv_sec - start_user_usage.tv_sec ) * 1E6 + ( end_user_usage.tv_usec - start_user_usage.tv_usec );
	double kernel_cpu_microtime = ( end_kernel_usage.tv_sec - start_kernel_usage.tv_sec ) * 1E6 + ( end_kernel_usage.tv_usec - start_kernel_usage.tv_usec );
	
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	
	//double latency_in_micro = (double)(micro_elapsed_time / OPERATIONS_CNT);
	double latency_in_micro = (double)(cumulative_latency / signaledPosts) / 1000;
	
	double mega_byte_per_sec = ((BUFFER_SIZE * OPERATIONS_CNT / 1E6 ) / (micro_elapsed_time / 1E6) );
	double operations_per_sec = OPERATIONS_CNT / (micro_elapsed_time / 1E6);
	double cpu_utilization = (user_cpu_microtime + kernel_cpu_microtime) / micro_elapsed_time;
	
	std::cout << "[Stat] Avg latency(u sec):   	" << latency_in_micro << std::endl; 
	std::cout << "[Stat] MegaByte per Sec:   	" << mega_byte_per_sec <<  std::endl;
	std::cout << "[Stat] Operations per Sec:   	" << operations_per_sec <<  std::endl;
	std::cout << "[Stat] CPU utilization:    	" << cpu_utilization << std::endl;
	std::cout << "[Stat] USER CPU utilization:    	" << user_cpu_microtime / micro_elapsed_time << std::endl;
	std::cout << "[Stat] KERNEL CPU utilization:    	" << kernel_cpu_microtime / micro_elapsed_time << std::endl;
	
	std::cout  << latency_in_micro << '\t' << mega_byte_per_sec << '\t' << operations_per_sec << '\t' << cpu_utilization << std::endl;
	
	return 0;
}
*/

int BenchmarkClient::start_benchmark(ClientContext *ctx) {
	int		server_num;
	int		abort_cnt = 0;
	bool	abort_flag;
	double cumulative_latency = 0;
	int signaledPosts = 0;
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec beforeSending, afterSending;				// for calculating TPMS
	
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock;
    
	struct rusage usage;
    struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;
	char temp_char;
	
	
	TEST_NZ (sock_sync_data (ctx->sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
	
	
	DEBUG_COUT ("[Info] Benchmark now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
    getrusage(RUSAGE_SELF, &usage);
    start_kernel_usage = usage.ru_stime;
    start_user_usage = usage.ru_utime;
	
	int offset;
	uint64_t *remote_address;
	
	int iteration = 0;
	while (iteration < OPERATIONS_CNT) {
		ctx->send_data_msg = rand() % (SERVER_REGION_SIZE - BUFFER_SIZE);
		
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->recv_data_mr, (uintptr_t)ctx->recv_data_msg, BUFFER_SIZE * sizeof(char)));
		
		DEBUG_COUT("[Info] Receive posted");
		
		if (iteration % 1000 == 0) {
			TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->send_data_mr, (uintptr_t)&ctx->send_data_msg, sizeof(int), true));
			DEBUG_COUT("[Sent] request to server");
		
			TEST_NZ (RDMACommon::poll_completion(ctx->cq));	// ack for SEND
			DEBUG_COUT("[Recv] Completion received");
		
		}
		else {
			TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->send_data_mr, (uintptr_t)&ctx->send_data_msg, sizeof(int), false));
			DEBUG_COUT("[Sent] request to server without completion");
		}
		
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));	// for RECEIVE
		DEBUG_COUT("[Recv] response from server");
	
		iteration++;
	}

    getrusage(RUSAGE_SELF, &usage);
    end_user_usage = usage.ru_utime;
    end_kernel_usage = usage.ru_stime;
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	double user_cpu_microtime = ( end_user_usage.tv_sec - start_user_usage.tv_sec ) * 1E6 + ( end_user_usage.tv_usec - start_user_usage.tv_usec );
	double kernel_cpu_microtime = ( end_kernel_usage.tv_sec - start_kernel_usage.tv_sec ) * 1E6 + ( end_kernel_usage.tv_usec - start_kernel_usage.tv_usec );
	
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	
	//double latency_in_micro = (double)(micro_elapsed_time / OPERATIONS_CNT);
	double latency_in_micro = (double)(cumulative_latency / signaledPosts) / 1000;
	
	double mega_byte_per_sec = ((BUFFER_SIZE * OPERATIONS_CNT / 1E6 ) / (micro_elapsed_time / 1E6) );
	double operations_per_sec = OPERATIONS_CNT / (micro_elapsed_time / 1E6);
	double cpu_utilization = (user_cpu_microtime + kernel_cpu_microtime) / micro_elapsed_time;
	
	std::cout << "[Stat] Avg latency(u sec):   	" << latency_in_micro << std::endl; 
	std::cout << "[Stat] MegaByte per Sec:   	" << mega_byte_per_sec <<  std::endl;
	std::cout << "[Stat] Operations per Sec:   	" << operations_per_sec <<  std::endl;
	std::cout << "[Stat] CPU utilization:    	" << cpu_utilization << std::endl;
	std::cout << "[Stat] USER CPU utilization:    	" << user_cpu_microtime / micro_elapsed_time << std::endl;
	std::cout << "[Stat] KERNEL CPU utilization:    	" << kernel_cpu_microtime / micro_elapsed_time << std::endl;
	
	std::cout  << latency_in_micro << '\t' << mega_byte_per_sec << '\t' << operations_per_sec << '\t' << cpu_utilization << std::endl;
	
	return 0;
}



int BenchmarkClient::start_client () {	
	ClientContext ctx[SERVER_CNT];
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed

		
	// then, client connext to servers
	for (int i = 0; i < SERVER_CNT; i++){
	    ctx[i].server_address = "";
		ctx[i].server_address += SERVER_ADDR[i];
		ctx[i].ib_port		  = IB_PORT[i];
		
		
		// Connect to servers
		TEST_NZ (establish_tcp_connection(ctx[i].server_address, TCP_PORT[i], &ctx[i].sockfd));
		DEBUG_COUT("[Conn] Connection established to server " << i);
	
		TEST_NZ (ctx[i].create_context());
		
		
		/************** For READ
		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(ctx[i].qp, ctx[i].recv_memory_mr, (uintptr_t)&ctx[i].recv_memory_msg, sizeof(struct MemoryKeys));
			
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));
		DEBUG_COUT("[Conn] QPed to server " << i);
				
	
		TEST_NZ(RDMACommon::poll_completion(ctx[i].cq));
		
		DEBUG_COUT("[Recv] buffers info from server " << i);
	
		// after receiving the message from the server, let's store its addresses in the context
		memcpy(&ctx[i].peer_data_mr,	&ctx[i].recv_memory_msg.peer_mr,	sizeof(struct ibv_mr));
		*/
		

		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));
		DEBUG_COUT("[Conn] QPed to server " << i);
		
	}
	DEBUG_COUT ("[Info] Successfully connected to all servers");
	
	start_benchmark(ctx);
	
	DEBUG_COUT("[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT("[Conn] Notified server " << i << " it's done");
		TEST_NZ ( ctx[i].destroy_context());
	}
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		BenchmarkClient::usage(argv[0]);
		return 1;
	}
	BenchmarkClient client;
	client.start_client();
	return 0;
}