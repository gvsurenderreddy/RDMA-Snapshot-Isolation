/*
 *	TradTrxManager.cpp
 *
 *	Created on: 9.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../two-sided/TradTrxManager.hpp"

#include "../util/utils.hpp"
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

//OrdersVersion*		TradTrxManager::orders_region		= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
//OrderLineVersion*		TradTrxManager::order_line_region	= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
//CCXactsVersion*		TradTrxManager::cc_xacts_region		= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];

//OrdersVersion*		TradTrxManager::orders_region		= new OrdersVersion[MAX_BUFFER_SIZE];		// TODO
//OrderLineVersion*	TradTrxManager::order_line_region	= new OrderLineVersion[MAX_BUFFER_SIZE];	// TODO
//CCXactsVersion*		TradTrxManager::cc_xacts_region		= new CCXactsVersion[MAX_BUFFER_SIZE];		// TODO

int	TradTrxManager::server_sockfd	= -1;		// Server's socket file descriptor
int	TradTrxManager::res_mng_socks[SERVER_CNT];

Timestamp	TradTrxManager::timestamp;
std::mutex 	TradTrxManager::timestamp_mutex;



// struct TradTrxManager::SharedContext TradTrxManager::s_ctx;


int TradTrxManager::open_device(struct ibv_context** ib_ctx) {
	struct	ibv_device **dev_list = NULL;
	struct	ibv_device *ib_dev = NULL;
	int		num_devices;

	// get device names in the system
	TEST_Z(dev_list = ibv_get_device_list (&num_devices));
	TEST_Z(num_devices); // if there isn't any IB device in host

	// select the first device
	const char *dev_name = strdup (ibv_get_device_name (dev_list[0]));
	TEST_Z(ib_dev = dev_list[0]);	// if the device wasn't found in host
	
	TEST_Z(*ib_ctx = ibv_open_device (ib_dev));		// get device handle

	// We are now done with device list, free it
	ibv_free_device_list (dev_list);
	dev_list = NULL;
	ib_dev = NULL;
	return 0;
}

int TradTrxManager::build_connection(int ib_port, struct ibv_context* ib_ctx,
struct ibv_port_attr* port_attr, struct ibv_pd **pd, struct ibv_cq **cq, int cq_size)
{
	struct	ibv_comp_channel *comp_channel;
	
	TEST_NZ (ibv_query_port (ib_ctx, ib_port, port_attr));

	TEST_Z(*pd = ibv_alloc_pd (ib_ctx));		// allocate Protection Domain

	// Create completion channel and completion queue
	TEST_Z(comp_channel = ibv_create_comp_channel(ib_ctx));
	
	TEST_Z(*cq = ibv_create_cq (ib_ctx, cq_size, NULL, comp_channel, 0));
	return 0;
}

void* TradTrxManager::handle_client(void *param) {
	TradTrxManagerContext *ctx = (TradTrxManagerContext *) param;
	char temp_char;
	
	// handles client's transactions one by one 
	TEST_NZ (start_transactions(*ctx));

	// once it's finished with the client, it syncs with client to ensure that client is over
	TEST_NZ (sock_sync_data (ctx->client_ctx.sockfd, 1, "W", &temp_char));	// just send a dummy char back and forth
		
	TEST_NZ (ctx->destroy_context());
	return NULL;
}

int TradTrxManager::acquire_commit_timestamp(Timestamp *commit_timestamp) {
	timestamp_mutex.lock();
	timestamp.value++;
	memcpy(commit_timestamp, &(TradTrxManager::timestamp), sizeof(Timestamp));
	timestamp_mutex.unlock();
}

int TradTrxManager::start_transactions(TradTrxManagerContext &ctx) {
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
	double avg_fetch_info = 0, avg_commit_ts = 0, avg_lock = 0, avg_decrement = 0, avg_respond = 0;
	
	// Server posts the RECEIVE job to be ready for the client's message containing its first request
	TEST_NZ (RDMACommon::post_RECEIVE(ctx.client_ctx.qp, ctx.client_ctx.commit_req_mr, (uintptr_t)&ctx.client_ctx.commit_request, sizeof(struct CommitRequest)));
	
	// Now that server is ready to receive its first commit request, it hand shakes to notify the client it's ready
	TEST_NZ (sock_sync_data (ctx.client_ctx.sockfd, 1, "T", &temp_char));	// just send a dummy char back and forth	
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	
	ctx.client_ctx.trx_num = 0;
	while (ctx.client_ctx.trx_num < TRANSACTION_CNT) {
		ctx.client_ctx.trx_num = ctx.client_ctx.trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Waiting for transaction #" << ctx.client_ctx.trx_num);
		
		// ************************************************************************
		// Step 1: Waits for client to post CommitRequest Job
		TEST_NZ (RDMACommon::poll_completion(ctx.client_ctx.cq));		// completion for post_RECEIVE
		
		DEBUG_COUT("[Recv] CommitRequest from client (" << get_full_desc(ctx.client_ctx) << ")");
		
		clock_gettime(CLOCK_REALTIME, &after_read_ts);
		
		
		// ************************************************************************
		// Step 2 (TODO: must be removed): TM fethces ItemInfo from corresponding RMs
		for (int i=0; i < SERVER_CNT; i++) {
			TradResManagerContext* res_ctx		= &ctx.res_mng_ctxs[i];
			res_ctx->item_info_request.I_ID		= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_I_ID;
				
			TEST_NZ (RDMACommon::post_RECEIVE(res_ctx->qp, res_ctx->item_info_res_mr, (uintptr_t)&res_ctx->item_info_response, sizeof(struct ItemInfoResponse)));
			TEST_NZ (RDMACommon::post_SEND(res_ctx->qp, res_ctx->item_info_req_mr, (uintptr_t)&res_ctx->item_info_request, sizeof(struct ItemInfoRequest), false));
			DEBUG_COUT("[Sent] ItemInfoReq for item " << res_ctx->item_info_request.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
		}
		
		for (int i=0; i < SERVER_CNT; i++) {
			TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
			TEST_NZ (RDMACommon::poll_completion(res_ctx->cq));		// completion for post_RECEIVE item_info_response
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
			TradResManagerContext* res_ctx	= &ctx.res_mng_ctxs[i];
			res_ctx->lock_request.I_ID		= ctx.client_ctx.commit_request.cart.cart_lines[i].SCL_I_ID;
				
			TEST_NZ (RDMACommon::post_RECEIVE(res_ctx->qp, res_ctx->lock_res_mr, (uintptr_t)&res_ctx->lock_response, sizeof(struct LockResponse)));
			TEST_NZ (RDMACommon::post_SEND(res_ctx->qp, res_ctx->lock_req_mr, (uintptr_t)&res_ctx->lock_request, sizeof(struct LockRequest), false));
			DEBUG_COUT("[Sent] Lock request for item " << res_ctx->lock_request.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");	
		}
		
		abort_flag = false;	
		for (int i=0; i < SERVER_CNT; i++) {
			TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
			TEST_NZ (RDMACommon::poll_completion(res_ctx->cq));		// completion for post_RECEIVE lock_response
			
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
				TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				if (successful_locking_servers[i] == true) 
					res_ctx->write_data_request.type	= WriteDataRequest::UNLOCK;
				else 
					res_ctx->write_data_request.type	= WriteDataRequest::DO_NOTHING;
			
				res_ctx->write_data_request.content.I_ID	= res_ctx->lock_request.I_ID;
					
				TEST_NZ (RDMACommon::post_SEND(res_ctx->qp, res_ctx->write_data_req_mr, (uintptr_t)&res_ctx->write_data_request, sizeof(struct WriteDataRequest), true));
			}
			for (int i=0; i < SERVER_CNT; i++) {
				TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				TEST_NZ (RDMACommon::poll_completion(res_ctx->cq));		// completion for post_SEND(WriteDataRequest)
				DEBUG_COUT(".... [Sent] Abort request for item " << res_ctx->write_data_request.content.I_ID << " to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
			}
			DEBUG_COUT("[Info] Transaction successfully aborted");
			abort_cnt++;
		}
		else {
		
			// ************************************************************************
			//	Step 5: Installs new versions on resource-managers (and relseases the locks on them)
			
			clock_gettime(CLOCK_REALTIME, &after_lock);
			
			DEBUG_COUT("[Info] All item locks successfully acquired");
			for (int i=0; i < SERVER_CNT; i++) {
				TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				
				res_ctx->write_data_request.type = WriteDataRequest::INSTALL_VERSION;
				res_ctx->write_data_request.content.write_ver_req.commit_ts = commit_timestamp;
				memcpy(&res_ctx->write_data_request.content.write_ver_req.cart_line, &ctx.client_ctx.commit_request.cart.cart_lines[i], sizeof(ShoppingCartLine));
				
				TEST_NZ (RDMACommon::post_SEND(res_ctx->qp, res_ctx->write_data_req_mr, (uintptr_t)&res_ctx->write_data_request, sizeof(struct WriteDataRequest), true));	
			}
			
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
			// int cc_index                            = (commit_timestamp.value - 1);
			int cc_index                               = (commit_timestamp.value - 1) % MAX_BUFFER_SIZE;
			cc_xacts_region[cc_index].write_timestamp  = commit_timestamp.value;
			cc_xacts_region[cc_index].cc_xacts.CX_O_ID = orders_region[ol_index].orders.O_ID;
			DEBUG_COUT("[Info] A new record to table CC_XACTS added");
			*/
			
			// Now collecting completions for WriteDataRequest
			for (int i=0; i < SERVER_CNT; i++) {
				TradResManagerContext* res_ctx = &ctx.res_mng_ctxs[i];
				TEST_NZ (RDMACommon::poll_completion(res_ctx->cq));
				DEBUG_COUT("[Sent] WriteDataRequest to RM #" << i << " (" << get_full_desc(*res_ctx) << ")");
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

		
		// re-arm the for the next commit request
		if (ctx.client_ctx.trx_num < TRANSACTION_CNT)
			TEST_NZ (RDMACommon::post_RECEIVE(ctx.client_ctx.qp, ctx.client_ctx.commit_req_mr, (uintptr_t)&ctx.client_ctx.commit_request, sizeof(struct CommitRequest)));
			
		TEST_NZ (RDMACommon::post_SEND (ctx.client_ctx.qp, ctx.client_ctx.commit_res_mr, (uintptr_t)&ctx.client_ctx.commit_response, sizeof(struct CommitResponse), true));
		TEST_NZ (RDMACommon::poll_completion(ctx.client_ctx.cq));		// completion for post_SEND
	
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
		}
	}
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	int committed_cnt = TRANSACTION_CNT - abort_cnt;
	
	avg_fetch_info	/= 1000;
	avg_commit_ts	/= 1000;
	avg_lock		/= 1000;
	avg_decrement	/= 1000;
	avg_respond		/= 1000;
	
	std::cout << std::endl; 
	std::cout << "[Stat] Avg fetch (u sec):         	" << (double)avg_fetch_info / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg commit ts (u sec):      	" << (double)avg_commit_ts / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg lock (u sec):            	" << (double)avg_lock / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg decrement (u sec):       	" << (double)avg_decrement / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg repond to client (u sec):	" << (double)avg_respond  / committed_cnt << std::endl;
	std::cout << "[Stat] Avg cumulative (u sec):     	" << (double)micro_elapsed_time / TRANSACTION_CNT << std::endl; 
	
	DEBUG_COUT (std::endl << "[Info] Successfully executed all transactions of client (" << get_full_desc(ctx.client_ctx) << ")");
	return 0;
}

std::string TradTrxManager::get_full_desc(BaseContext &ctx) {
	std::stringstream sstm;
	//sstm << "ip: " << ctx->client_ip << ", port: " << ctx->client_port  << ", sock: " << ctx->client_sockfd; 
	sstm << ctx.sockfd;
	return sstm.str();
}

int TradTrxManager::destroy_resources () {
	for (int i = 0; i < SERVER_CNT; i++)
		if (res_mng_socks[i] >= 0)
			TEST_NZ (close (res_mng_socks[i]));
	
	//if (s_ctx.ib_ctx)
	//	TEST_NZ (ibv_close_device (s_ctx.ib_ctx));
	
	close(server_sockfd);	// close the socket
	return 0;
}

int TradTrxManager::initialize_data_structures(){
	timestamp.value = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("[Info] Timestamp set to " << timestamp.value);
	return 0;
}

void TradTrxManager::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " starts a server and wait for connection" << std::endl;
}

int TradTrxManager::start_server () {	
	TEST_NZ(initialize_data_structures());
	
	struct sockaddr_in returned_addr;
	socklen_t len = sizeof(returned_addr);
	pthread_t master_threads[CLIENTS_CNT];
	std::string res_mng_ip[SERVER_CNT];
	int res_mng_port[SERVER_CNT];
	struct TradTrxManagerContext ctx[CLIENTS_CNT];
	
	
	// Create the shared context
	// TEST_NZ (TradTrxManager::open_device(&s_ctx.ib_ctx));
	
	// Call socket(), bing() and listen()
	TEST_NZ (server_socket_setup(&server_sockfd, CLIENTS_CNT + SERVER_CNT));
	
	// accept connections from Resource-managers
	std::cout << "[Info] Waiting for " << SERVER_CNT << " RM(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;
	for (int s = 0; s < SERVER_CNT; s++) {
		res_mng_socks[s]  = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (res_mng_socks[s] < 0){ 
			std::cerr << "ERROR on accept() for RM #" << s << std::endl;
			return -1;
		}
		res_mng_ip[s] = "";
		res_mng_ip[s] += std::string(inet_ntoa (returned_addr.sin_addr));
		res_mng_port[s]	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received RM #" << s <<  " (" << res_mng_ip[s] << ", " << res_mng_port[s] << ") on sock " << res_mng_socks[s] << std::endl;
	}
	std::cout << "[Info] Established connection to all " << SERVER_CNT << " resource-manager(s)." << std::endl; 
	
	
	// accept connections from clients
	std::cout << std::endl << "[Info] Waiting for " << CLIENTS_CNT << " client(s) on port " << TRX_MANAGER_TCP_PORT << std::endl;	
	for (int c = 0; c < CLIENTS_CNT; c++){
		ctx[c].client_ctx.ib_port = TRX_MANAGER_IB_PORT;
		ctx[c].client_ctx.sockfd = accept (server_sockfd, (struct sockaddr *) &returned_addr, &len);
		if (ctx[c].client_ctx.sockfd < 0) {
			std::cerr << "ERROR on accept() client #" << c << std::endl;
			return -1;
		}
		ctx[c].client_ctx.client_ip = "";
		ctx[c].client_ctx.client_ip	+= std::string(inet_ntoa (returned_addr.sin_addr));
		ctx[c].client_ctx.client_port	= (int) ntohs(returned_addr.sin_port);
		std::cout << "[Conn] Received client #" << c << " (" << ctx[c].client_ctx.client_ip << ", " << ctx[c].client_ctx.client_port << ") on sock " << ctx[c].client_ctx.sockfd << std::endl;
		
		// connecting the QPs to resource-managers
		for (int s = 0; s < SERVER_CNT; s++) {
			ctx[c].res_mng_ctxs[s].ib_port 		= TRX_MANAGER_IB_PORT;
			ctx[c].res_mng_ctxs[s].client_ip	= "";
			ctx[c].res_mng_ctxs[s].client_ip	+= res_mng_ip[s];
			ctx[c].res_mng_ctxs[s].client_port	= res_mng_port[s];
			ctx[c].res_mng_ctxs[s].sockfd		= res_mng_socks[s];
		}
		ctx[c].create_context();	
		pthread_create(&master_threads[c], NULL, TradTrxManager::handle_client, &ctx[c]);
	}
	std::cout << "[Info] Established connection to all " << CLIENTS_CNT << " client(s)." << std::endl; 
	
	
	//wait for handlers to finish
	for (int i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	
	// close server socket
	TEST_NZ(destroy_resources());
	std::cout << "[Info] Server is done and destroyed its resources!" << std::endl;
}


int main (int argc, char *argv[]) {
	if (argc != 1) {
		TradTrxManager::usage(argv[0]);
		return 1;
	}
	TradTrxManager server;
	server.start_server();
	return 0;
}
