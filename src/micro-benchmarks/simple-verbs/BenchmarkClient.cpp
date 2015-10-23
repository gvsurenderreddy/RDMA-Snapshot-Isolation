/*
 *	BenchmarkClient.cpp
 *
 *	Created on: 26.March.2015
 *	Author: Erfan Zamanian
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


#define CLASS_NAME	"BenchClient"


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

int BenchmarkClient::perform_operation() {
	 uint64_t *lookup_add;
	if (benchmark_config::MEMORY_ACCESS == benchmark_config::MEMORY_ACCESS_ENUM::SHARED)
		// In SHARED memory access, all clients access the same memory region (the first 8 bytes)
		lookup_add =  (uint64_t *)((uint64_t)ctx.peer_data_mr.addr);
	else {
		// In DEDICATED memory access, each client accesses a dedicated memory region on the server
		size_t offset = client_num * sizeof(uint64_t);
		lookup_add =  (uint64_t *)(offset + ((uint64_t)ctx.peer_data_mr.addr));
	}

	if (benchmark_config::VERB_TYPE == benchmark_config::VERB_TYPE_ENUM::READ){
		TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				ctx.qp,
				ctx.recv_data_mr,
				(uint64_t)ctx.recv_data_msg,
				&(ctx.peer_data_mr),
				(uint64_t)lookup_add,
				benchmark_config::BUFFER_SIZE,
				true));
		DEBUG_COUT(CLASS_NAME, __func__, "[READ] Server's buffer read");
	}
	else if (benchmark_config::VERB_TYPE == benchmark_config::VERB_TYPE_ENUM::WRITE) {
		TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx.qp,
				ctx.send_data_mr,
				(uint64_t)ctx.send_data_msg,
				&(ctx.peer_data_mr),
				(uint64_t)lookup_add,
				benchmark_config::BUFFER_SIZE,
				true));
		DEBUG_COUT(CLASS_NAME, __func__, "[WRIT] Server's buffer written to");
	}
	else if (benchmark_config::VERB_TYPE == benchmark_config::VERB_TYPE_ENUM::CAS) {
		uint64_t	expected_value = 0ULL;
		uint64_t	new_value = 0ULL;

		TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(
				ctx.qp,
				ctx.send_data_mr,
				(uint64_t)ctx.send_data_msg,
				&(ctx.peer_data_mr),
				(uint64_t)lookup_add,
				(uint32_t)sizeof(uint64_t),
				(uint64_t)expected_value,
				(uint64_t)new_value));
		DEBUG_COUT(CLASS_NAME, __func__, "[CAS] Server's buffer CASed");
	}
	else if (benchmark_config::VERB_TYPE == benchmark_config::VERB_TYPE_ENUM::FA) {
		TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(
				ctx.qp,
				ctx.send_data_mr,
				(uint64_t)ctx.send_data_mr,
				&(ctx.peer_data_mr),
				(uint64_t)lookup_add,
				(uint64_t)1ULL,
				(uint32_t)sizeof(uint64_t)));
		DEBUG_COUT(CLASS_NAME, __func__, "[FADD] Server's buffer FADDed");
	}

	TEST_NZ (RDMACommon::poll_completion(ctx.send_cq));
	return 0;
}


int BenchmarkClient::start_benchmark() {
	// int signaledPosts = 0;
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	//struct timespec beforeSending, afterSending;				// for calculating TPMS

	//struct rusage usage;
	// struct timeval start_user_usage, start_kernel_usage, end_user_usage, end_kernel_usage;

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Benchmark now gets started");

	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	//getrusage(RUSAGE_SELF, &usage);
	//start_kernel_usage = usage.ru_stime;
	//start_user_usage = usage.ru_utime;

	for (int i= 0; i < benchmark_config::OPERATIONS_CNT; i++)
		perform_operation();


	// getrusage(RUSAGE_SELF, &usage);
	// end_user_usage = usage.ru_utime;
	// end_kernel_usage = usage.ru_stime;

	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	//double user_cpu_microtime = ( end_user_usage.tv_sec - start_user_usage.tv_sec ) * 1E6 + ( end_user_usage.tv_usec - start_user_usage.tv_usec );
	//double kernel_cpu_microtime = ( end_kernel_usage.tv_sec - start_kernel_usage.tv_sec ) * 1E6 + ( end_kernel_usage.tv_usec - start_kernel_usage.tv_usec );


	double micro_elapsed_time = ((double) ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + (double)( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;

	double latency_in_micro = (double)(micro_elapsed_time / benchmark_config::OPERATIONS_CNT);
	//double latency_in_micro = (double)(cumulative_latency / signaledPosts) / 1000;

	double mega_byte_per_sec = ((benchmark_config::BUFFER_SIZE * benchmark_config::OPERATIONS_CNT / 1E6 ) / (micro_elapsed_time / 1E6) );
	double operations_per_sec = benchmark_config::OPERATIONS_CNT / (micro_elapsed_time / 1E6);
	//double cpu_utilization = (user_cpu_microtime + kernel_cpu_microtime) / micro_elapsed_time;

	std::cout << "[Stat] Avg latency(u sec):   	" << latency_in_micro << std::endl; 
	std::cout << "[Stat] MegaByte per Sec:   	" << mega_byte_per_sec <<  std::endl;
	std::cout << "[Stat] Operations per Sec:   	" << operations_per_sec <<  std::endl;

	//std::cout << "[Stat] CPU utilization:    	" << cpu_utilization << std::endl;
	//std::cout << "[Stat] USER CPU utilization:    	" << user_cpu_microtime / micro_elapsed_time << std::endl;
	//std::cout << "[Stat] KERNEL CPU utilization:    	" << kernel_cpu_microtime / micro_elapsed_time << std::endl;
	return 0;
}



int BenchmarkClient::start_client () {
	srand ((unsigned int)generate_random_seed());		// initialize random seed

	// Connect to servers
	TEST_NZ (establish_tcp_connection(ctx.server_address, config::TCP_PORT[0], &ctx.sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server ");

	TEST_NZ (ctx.create_context());

	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(ctx.qp, ctx.recv_memory_mr, (uintptr_t)&ctx.recv_memory_msg, sizeof(struct MemoryKeys));

	TEST_NZ (RDMACommon::connect_qp (&(ctx.qp), ctx.ib_port, ctx.port_attr.lid, ctx.sockfd));

	TEST_NZ(RDMACommon::poll_completion(ctx.recv_cq));
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers handlers received from server ");

	// after receiving the message from the server, let's store its addresses in the context
	memcpy(&ctx.peer_data_mr,	&ctx.recv_memory_msg.peer_mr,	sizeof(struct ibv_mr));
	client_num = ctx.recv_memory_msg.client_num;

	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server ");

	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Successfully connected to the server");

	start_benchmark();

	char temp_char;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is done, and is ready to destroy its resources!");
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified server it's done");
	TEST_NZ (ctx.destroy_context());

	return 0;
}

BenchmarkClient::BenchmarkClient() {
	ctx.server_address = "";
	ctx.server_address += config::SERVER_ADDR[0];
	ctx.ib_port		  = config::IB_PORT[0];
}
