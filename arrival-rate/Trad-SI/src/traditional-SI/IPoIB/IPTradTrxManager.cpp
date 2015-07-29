/*
 *	IPTradTrxManager.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: erfanz
 */

#include "IPTradTrxManager.hpp"
#include "../../util/utils.hpp"
#include <stdio.h>
#include <string.h>
#include <sstream>	// for stringstream
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <netinet/tcp.h>	// for setsockopt



void* IPTradTrxManager::handle_client(void *param) {
	IPTradTrxManagerContext *ctx = (IPTradTrxManagerContext *) param;
	char temp_char;
	
	// handles client's transactions one by one 
	TEST_NZ (start_transactions(*ctx));

	// once it's finished with the client, it syncs with client to ensure that client is over
	TEST_NZ (sock_sync_data (ctx->client_ctx.sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
		
	TEST_NZ (ctx->destroy_context());
	return NULL;
}

int IPTradTrxManager::start_transactions(IPTradTrxManagerContext &ctx) {
	
	char	temp_char;
	int		new_order_index;
	int		new_orderline_index;
	int		new_ccxacts_index;
	bool	abort_flag;
    bool	successful_locking_servers[SERVER_CNT] = { false };		// initializes the entire array to false 
	Timestamp read_timestamp;
	Timestamp commit_timestamp;
	int abort_cnt = 0;	// number of aborts
	
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
		
	struct timespec after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_respond_to_client;
	double avg_fetch_info = 0, avg_commit_ts = 0, avg_lock = 0, avg_decrement = 0, avg_respond = 0, avg_useful_time = 0;
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer

	
	ctx.client_ctx.trx_num = 0;
	while (ctx.client_ctx.trx_num < TRANSACTION_CNT) {
		
		ctx.client_ctx.trx_num = ctx.client_ctx.trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Waiting for transaction #" << ctx.client_ctx.trx_num);
		
		
		// ************************************************************************
		// Step 1: Waits for client to post CommitRequest Job
		TEST_NZ (sock_read(ctx.client_ctx.sockfd, (char *)&(ctx.client_ctx.commit_request), sizeof(struct CommitRequest)));	
				
		DEBUG_COUT ("[Recv] CommitRequest from client (" << get_full_desc(ctx.client_ctx) << ")");
	
		clock_gettime(CLOCK_REALTIME, &after_read_ts);
	
		// ************************************************************************
		// Step 2 (TODO: must be removed): TM fethces ItemInfo from corresponding RMs
		for (int i = 0; i < SERVER_CNT; i++) {
			IPTradResManagerContext* res_ctx	= &ctx.res_mng_ctxs[i];
			res_ctx->item_info_request.I_ID		= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_I_ID;
	
			TEST_NZ (sock_write(res_ctx->sockfd, (char *)&(res_ctx->item_info_request), sizeof(struct ItemInfoRequest)));			
			DEBUG_COUT("[Sent] ItemInfoReq for item " << res_ctx->item_info_request.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
		}
		
		for (int i=0; i < SERVER_CNT; i++) {
			IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
			
			TEST_NZ (sock_read(res_ctx->sockfd, (char *)&(res_ctx->item_info_response), sizeof(struct ItemInfoResponse)));	
			DEBUG_COUT("[Recv] ItemInfo for item " << res_ctx->item_info_response.item_version.item.I_ID << " from RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
		}
		clock_gettime(CLOCK_REALTIME, &after_fetch_info);
				
		
		// ************************************************************************
		//	Step 3: Acquires commmit timestamp
		TEST_NZ (acquire_commit_timestamp(&commit_timestamp));
		DEBUG_COUT("[Info] Acquired commit timestamp: " << commit_timestamp.value);
		
		clock_gettime(CLOCK_REALTIME, &after_commit_ts);
		
		
		// ************************************************************************
		//	Step 4: Acquires locks on items on resource-managers
		for (int i=0; i < SERVER_CNT; i++) {
			IPTradResManagerContext* res_ctx	= &ctx.res_mng_ctxs[i];
			res_ctx->lock_request.I_ID		= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_I_ID;
			
			TEST_NZ (sock_write(res_ctx->sockfd, (char *)&(res_ctx->lock_request), sizeof(struct LockRequest)));
			DEBUG_COUT("[Sent] Lock request for item " << res_ctx->lock_request.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
		}
		
		abort_flag = false;	
		for (int i=0; i < SERVER_CNT; i++) {
			IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
			
			TEST_NZ (sock_read(res_ctx->sockfd, (char *)&(res_ctx->lock_response), sizeof(struct LockResponse)));			
			
			if (res_ctx->lock_response.was_successful) {
				DEBUG_COUT("[Recv] Successful lock on item " << res_ctx->lock_request.I_ID << " from RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
				successful_locking_servers[i] = true;
			}
			else {
				DEBUG_COUT("[Recv] Unsuccessful lock on item " << res_ctx->lock_request.I_ID << " from RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
				successful_locking_servers[i] = false;
				abort_flag = true;
			}
		}
		
		if (abort_flag) {
			DEBUG_COUT("[Info] Successful locks must be aborted:");
			// must revert all the successful locks before ABORT
			for (int i = 0; i < SERVER_CNT; i++) {
				// server only sends revert request to RMs with successfully locked items 
				IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				res_ctx->write_data_request.content.I_ID	= res_ctx->lock_request.I_ID;
				
				if (successful_locking_servers[i] == true) 
					res_ctx->write_data_request.type	= WriteDataRequest::UNLOCK;
				else 
					res_ctx->write_data_request.type	= WriteDataRequest::DO_NOTHING;
				
				
				TEST_NZ (sock_write(res_ctx->sockfd, (char *)&(res_ctx->write_data_request), sizeof(struct WriteDataRequest)));		
				DEBUG_COUT(".... [Sent] Abort request for item " << res_ctx->write_data_request.content.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
			}
			
			for (int i = 0; i < SERVER_CNT; i++) {
				// server only sends revert request to RMs with successfully locked items 
				IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				TEST_NZ (sock_read(res_ctx->sockfd, (char *)&(res_ctx->write_data_response), sizeof(struct WriteDataResponse)));		
				DEBUG_COUT(".... [Recv] Abort response for item " << res_ctx->write_data_response.I_ID << " from RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
			}
			
			DEBUG_COUT("[Info] Transaction successfully aborted");
			abort_cnt++;
		}
		else {
			
			/*
			// Meanwhile, the server stores ORDER, ORDERLINE and CCXACTS information
			// Insert ORDER
			//int o_index = commit_timestamp.value - 1;
			int o_index = (commit_timestamp.value - 1) % MAX_BUFFER_SIZE;
			orders_region[o_index].write_timestamp	= commit_timestamp.value;
			orders_region[o_index].orders.O_ID		= commit_timestamp.value;
			DEBUG_COUT("[Info] A new record to table ORDERS added");
			
			
			// Insert ORDERLINE(s) 
			int ol_index;
			for (int i=0; i < ORDERLINE_PER_ORDER; i++) {
				// ol_index = i + (commit_timestamp.value - 1) * ORDERLINE_PER_ORDER;
				ol_index = (i + (commit_timestamp.value - 1) * ORDERLINE_PER_ORDER) % MAX_BUFFER_SIZE;
				order_line_region[ol_index].write_timestamp		= commit_timestamp.value;
				order_line_region[ol_index].order_line.OL_ID	= ol_index;
				order_line_region[ol_index].order_line.OL_O_ID	= orders_region[o_index].orders.O_ID;
				order_line_region[ol_index].order_line.OL_I_ID	= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_I_ID;
				order_line_region[ol_index].order_line.OL_QTY	= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_QTY;
			}
			DEBUG_COUT("[Info] A new record to table ORDERLINE added");
			
			
			// Insert CCXACTS
			// int cc_index = (commit_timestamp.value - 1);
			int cc_index = (commit_timestamp.value - 1) % MAX_BUFFER_SIZE;
			cc_xacts_region[cc_index].write_timestamp		= commit_timestamp.value;
			cc_xacts_region[cc_index].cc_xacts.CX_O_ID		= orders_region[ol_index].orders.O_ID;
			DEBUG_COUT("[Info] A new record to table CC_XACTS added");
			*/
		
			// ************************************************************************
			//	Step 5: Installs new versions on resource-managers (and relseases the locks on them)
			
			
			clock_gettime(CLOCK_REALTIME, &after_lock);
			
			DEBUG_COUT("[Info] All item locks successfully acquired");
			for (int i=0; i < SERVER_CNT; i++) {
				IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				res_ctx->write_data_request.type = WriteDataRequest::INSTALL_VERSION;
				res_ctx->write_data_request.content.write_ver_req.commit_ts = commit_timestamp;
				memcpy(&res_ctx->write_data_request.content.write_ver_req.cart_line, &ctx.client_ctx.commit_request.cart.cart_lines[i], sizeof(ShoppingCartLine));
				
				TEST_NZ (sock_write(res_ctx->sockfd, (char *)&(res_ctx->write_data_request), sizeof(struct WriteDataRequest)));
				DEBUG_COUT("[Sent] WriteDataRequest to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
			}
			
			for (int i = 0; i < SERVER_CNT; i++) {
				// server only sends revert request to RMs with successfully locked items 
				IPTradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				TEST_NZ (sock_read(res_ctx->sockfd, (char *)&(res_ctx->write_data_response), sizeof(struct WriteDataResponse)));		
				DEBUG_COUT(".... [Recv] WriteDataResponse for item " << res_ctx->write_data_response.I_ID << " from RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
			}
		}
			
		DEBUG_COUT("[Info] All WriteDataRequests sent");	
		clock_gettime(CLOCK_REALTIME, &after_decrement);
		
			
		// ************************************************************************
		//	Step 6: Returns the commit result to client
		if (abort_flag)
			ctx.client_ctx.commit_response.commit_outcome = CommitResponse::ABORTED;
		else
			ctx.client_ctx.commit_response.commit_outcome = CommitResponse::COMMITTED;

		
		TEST_NZ (sock_write(ctx.client_ctx.sockfd, (char *)&(ctx.client_ctx.commit_response), sizeof(struct CommitResponse)));	
		DEBUG_COUT("[Sent] Commit result to client (" << get_full_desc(ctx.client_ctx) << ")"); 
		
		clock_gettime(CLOCK_REALTIME, &after_respond_to_client);
		
		if (abort_flag == false){
			double t = ( after_fetch_info.tv_sec - after_read_ts.tv_sec ) * 1E9 + ( after_fetch_info.tv_nsec - after_read_ts.tv_nsec );
			avg_fetch_info += t;
			
			t = ( after_commit_ts.tv_sec - after_fetch_info.tv_sec ) * 1E9 + ( after_commit_ts.tv_nsec - after_fetch_info.tv_nsec );
			avg_commit_ts += t;
			
			t = ( after_lock.tv_sec - after_commit_ts.tv_sec ) * 1E9 + ( after_lock.tv_nsec - after_commit_ts.tv_nsec );
			avg_lock += t;
			
			t = ( after_decrement.tv_sec - after_lock.tv_sec ) * 1E9 + ( after_decrement.tv_nsec - after_lock.tv_nsec );
			avg_decrement += t;
			
			t = ( after_respond_to_client.tv_sec - after_decrement.tv_sec ) * 1E9 + ( after_respond_to_client.tv_nsec - after_decrement.tv_nsec );
			avg_respond += t;
			
			t = ( after_respond_to_client.tv_sec - after_read_ts.tv_sec ) * 1E9 + ( after_respond_to_client.tv_nsec - after_read_ts.tv_nsec );
			avg_useful_time += t;
		}
	}
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	int committed_cnt = TRANSACTION_CNT - abort_cnt;

	avg_fetch_info /= 1000;
	avg_commit_ts /= 1000;
	avg_lock /= 1000;
	avg_decrement /= 1000;
	avg_respond /= 1000;
	
	std::cout << std::endl << "Total Elapsed time (u sec): " << micro_elapsed_time << std::endl;

	std::cout << "[Stat] Avg fetch (u sec):           	" << (double)avg_fetch_info / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg commit ts (u sec):       	" << (double)avg_commit_ts / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg lock (u sec):            	" << (double)avg_lock / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg decrement (u sec):       	" << (double)avg_decrement / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg respond to client (u sec):	" << (double)avg_respond  / committed_cnt << std::endl;
	std::cout << "[Stat] Avg cumulative (u sec):       	" << (double)micro_elapsed_time / TRANSACTION_CNT << std::endl;
	std::cout << "[Stat] Avg cumulative (u sec):       	" << (double)micro_elapsed_time / TRANSACTION_CNT << std::endl; 
	std::cout << "[Stat] Avg useful time (u sec):		" << (double)(avg_useful_time / 1000) / committed_cnt << std::endl;
	 
	
	DEBUG_COUT (std::endl << "[Info] Successfully executed all transactions of client (" << get_full_desc(ctx.client_ctx) << ")");
	
	return 0;
}

int IPTradTrxManager::start_server () {	
	struct sockaddr_in serv_addr, returned_addr;
	socklen_t len = sizeof(returned_addr);
	pthread_t master_threads[CLIENTS_CNT];
	struct IPTradTrxManagerContext ctx[CLIENTS_CNT];
	char temp_char;
	
	// First load the data
	TEST_NZ (initialize_data_structures());
	
	
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
	TEST_NZ(listen (server_sockfd, CLIENTS_CNT*SERVER_CNT + CLIENTS_CNT));
	
	
	// accept connections from Resource-managers
	std::cout << "[Info] Waiting for " << SERVER_CNT << " RM(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;
	for (int s = 0; s < SERVER_CNT; s++) {
		for (int c = 0; c < CLIENTS_CNT; c++) {
			ctx[c].res_mng_ctxs[s].sockfd  = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
			if (res_mng_socks[s] < 0) { 
				std::cerr << "ERROR on accept() for RM #" << s << std::endl;
				return -1;
			}
			ctx[c].res_mng_ctxs[s].client_ip = "";
			ctx[c].res_mng_ctxs[s].client_ip += std::string(inet_ntoa (returned_addr.sin_addr));
			ctx[c].res_mng_ctxs[s].client_port	= (int) ntohs(returned_addr.sin_port);
		
			std::cout << "[Conn] For client " << c << ", received RM " << s <<  " (" << ctx[c].res_mng_ctxs[s].client_ip
				<< ", " << ctx[c].res_mng_ctxs[s].client_port << ") on sock " << ctx[c].res_mng_ctxs[s].sockfd << std::endl;
		}
		std::cout << "[Info] Established connection to all " << CLIENTS_CNT << " clients for RM #" << s << std::endl; 
	}
	std::cout << "[Info] All wires to RMs are in place. Now it's safe to add nodes" << std::endl;
	
	
	// accept connections from clients
	std::cout << std::endl << "[Info] Waiting for " << CLIENTS_CNT << " client(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;	
	for (int c = 0; c < CLIENTS_CNT; c++){
		ctx[c].client_ctx.sockfd = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[c].client_ctx.sockfd < 0) {
			std::cerr << "ERROR on accept() client " << c << std::endl;
			return -1;
		}
		ctx[c].client_ctx.client_ip = "";
		ctx[c].client_ctx.client_ip	+= std::string(inet_ntoa (returned_addr.sin_addr));
		ctx[c].client_ctx.client_port	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client " << c << " (" << ctx[c].client_ctx.client_ip << ", " << ctx[c].client_ctx.client_port << ") on sock " << ctx[c].client_ctx.sockfd << std::endl;		
	}
	
	std::cout << "[Info] Established connection to all " << CLIENTS_CNT << " client(s) and " << SERVER_CNT << " RM(s)." << std::endl; 
	
	
	for (int c = 0; c < CLIENTS_CNT; c++) {
		pthread_create(&master_threads[c], NULL, IPTradTrxManager::handle_client, &ctx[c]);
	}
	
	//wait for handlers to finish
	for (int i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	
	// close server socket
	TEST_NZ (destroy_resources());
	std::cout << "[Info] Server is done and destroyed its resources!" << std::endl;
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		IPTradTrxManager::usage(argv[0]);
		return 1;
	}
	IPTradTrxManager server;
	server.start_server();
	return 0;
}