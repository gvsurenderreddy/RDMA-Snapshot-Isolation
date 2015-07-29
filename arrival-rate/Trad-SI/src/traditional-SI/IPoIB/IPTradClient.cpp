/*
 *	IPTradClient.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: erfanz
 */

#include "IPTradClient.hpp"
#include "../../util/utils.hpp"
#include "../../util/zipf.hpp"
#include "../../util/RDMACommon.hpp"
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

int IPTradClient::start_transaction(IPTradClientContext &ctx) {
	int abort_cnt = 0;
	struct timespec firstRequestTime, lastRequestTime; // for calculating TPMS
	char temp_char;
	
	struct timespec before_commit_request, after_commit_response, temp_ts;
	double avg_useful_time = 0;
	bool 	first_transaction_flag = true;
	
	double	TIME_BETWEEN_ARRIVAL_TRX = (1 / TRX_ARRIVAL_RATE) * 1E6;			// in microsecond
	
	int sleeptime = (rand() % 2000000);
	usleep(sleeptime);
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	
	ctx.trx_num = 0;
	while (ctx.trx_num  <  TRANSACTION_CNT) {
		// Adjust the transaction rate to match the given arrival rate
		if (!first_transaction_flag) {
			clock_gettime(CLOCK_REALTIME, &temp_ts);
			double last_transaction_elapsed_time = ( temp_ts.tv_sec - before_commit_request.tv_sec ) * 1E6 + (( temp_ts.tv_nsec - before_commit_request.tv_nsec ) / 1E3);
			if (last_transaction_elapsed_time < TIME_BETWEEN_ARRIVAL_TRX) {
				usleep (TIME_BETWEEN_ARRIVAL_TRX - last_transaction_elapsed_time);
			}
		}
		else first_transaction_flag = false;
		
		
		clock_gettime(CLOCK_REALTIME, &before_commit_request);
		
		
		ctx.trx_num = ctx.trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Submitting transaction #" << ctx.trx_num);

		// ************************************************************************
		//	Client fills their shopping cart
		fill_shopping_cart(&ctx.commit_request.cart, ctx.sockfd);
			
		
		// ************************************************************************
		//	Client requests for commit
		TEST_NZ (sock_write(ctx.sockfd, (char *)&(ctx.commit_request), sizeof(struct CommitRequest)));	
		DEBUG_COUT("[Sent] CommitRequest to TM");
		
		
		// ************************************************************************
		//	Clients waits for the outcome of the commit
		TEST_NZ (sock_read(ctx.sockfd, (char *)&(ctx.commit_response), sizeof(struct CommitResponse)));	
		
		clock_gettime(CLOCK_REALTIME, &after_commit_response);

		
		if (ctx.commit_response.commit_outcome == CommitResponse::COMMITTED) {
			DEBUG_COUT("[Recv] CommitResponse (result: committed)");
			double t = ( after_commit_response.tv_sec - before_commit_request.tv_sec ) * 1E9 + ( after_commit_response.tv_nsec - before_commit_request.tv_nsec );
			avg_useful_time += t;		
		}
		else {
			DEBUG_COUT("[Recv] CommitResponse (result: aborted)");
			abort_cnt++;
		}
	}

	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	
	double nano_elapsed_time = ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec );
	
	int committed_cnt = TRANSACTION_CNT - abort_cnt;
	double success_rate = (double)committed_cnt /  TRANSACTION_CNT;
	//std::cout << "[Stat] " << committed_cnt << " committed, " << abort_cnt << " aborted,   success rate = " <<  '\t' << success_rate << std::endl;
	
	double T_P_MILISEC = (double)(committed_cnt / (double)(nano_elapsed_time / 1000000));
	//std::cout << std::endl << "[Stat] Transaction per millisec: " <<  T_P_MILISEC << std::endl;
	
	
	std::cout << "Avg useful time (u sec):	" << (double)(avg_useful_time / 1000) / committed_cnt
		<< "\t\tTransaction per millisec:	" <<  T_P_MILISEC
		<< "\t\tCommited: " << committed_cnt << ", Aborted: " << abort_cnt << ". Success rate:	" << success_rate << std::endl;
	
	
	
	return 0;
}

int IPTradClient::start_client () {	
	IPTradClientContext ctx;
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed
	zipf_initialize(SKEWNESS_IN_ITEM_ACCESS, ITEM_PER_SERVER);
	
	int item_id;
	for (int i = 0; i < TRANSACTION_CNT; i++){
		for (int j = 0; j < ORDERLINE_PER_ORDER; j++){
			item_id	 = (j * ITEM_PER_SERVER) + zipf_get_sample(SKEWNESS_IN_ITEM_ACCESS, ITEM_PER_SERVER) - 1;	// generating in the range 0 to ITEM_CNT
			preselected_item_ids[j].push_back(item_id);
		}
	}
		
	TEST_NZ (establish_tcp_connection(TRX_MANAGER_ADDR.c_str(), TRX_MANAGER_TCP_PORT, &(ctx.sockfd)));
	
	DEBUG_COUT("[Comm] Client connected to TM on sock " << ctx.sockfd);

	TEST_NZ (ctx.create_context());
	
	start_transaction(ctx);
	
	// Sync so server will know that client is done mucking with its memory
	DEBUG_COUT("[Info] Client is done, and is ready to destroy its resources!");
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	TEST_NZ(ctx.destroy_context());
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		IPTradClient::usage(argv[0]);
		return 1;
	}
	IPTradClient client;
	client.start_client();
	return 0;
}