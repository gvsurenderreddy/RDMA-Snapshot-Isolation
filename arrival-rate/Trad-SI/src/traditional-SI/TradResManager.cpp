/*
 *	TradResManager.hpp
 *
 *	Created on: 13.Feb.2015
 *	Author: erfanz
 */


#include "TradResManager.hpp"
#include "../util/utils.hpp"
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


ItemVersion*	TradResManager::items_region	= new ItemVersion[ITEM_CNT * MAX_ITEM_VERSIONS];
std::mutex		TradResManager::item_lock[ITEM_CNT];
int				TradResManager::tcp_port;
int				TradResManager::res_mng_sockfd;


int TradResManager::destroy_resources () {
	delete[](TradResManager::items_region);
	if (TradResManager::res_mng_sockfd >= 0){
		TEST_NZ (close (TradResManager::res_mng_sockfd));
	}
	return 0;
}

int TradResManager::initialize_data_structures() {
	TEST_NZ (load_tables_from_files(items_region));
	DEBUG_COUT ("[Info] Tables loaded successfully");
	return 0;
}

void TradResManager::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " <i = server_num>" << std::endl;
	std::cout << "starts a RM server and waits for connection on port Config.TCP_PORT[i]" << std::endl;
	std::cout << "(valid range of i: 0, 1, ..., [Config.SERVER_CNT - 1])" << std::endl;
}

int TradResManager::lock_item(int I_ID) {
	return item_lock[I_ID].try_lock();
}

int TradResManager::unlock_item(int I_ID) {
	item_lock[I_ID].unlock();
	return 0;
}

int TradResManager::decremenet_item_stock(int item_id, int quantity, Timestamp* commit_timestamp) {
	TradResManager::items_region[item_id].write_timestamp	= commit_timestamp->value;
	TradResManager::items_region[item_id].item.I_STOCK		-= quantity;
	
	if (TradResManager::items_region[item_id].item.I_STOCK < 10)
		TradResManager::items_region[item_id].item.I_STOCK += 20;
	
	return 0;
}

void* TradResManager::handle_trx_mng_thread(void *param) {
	TradResManagerContext *ctx = (TradResManagerContext *)param;

	ShoppingCartLine	*cart_line;
	Timestamp			*commit_ts;
	
	ctx->trx_num = 0;
	while(ctx->trx_num < TRANSACTION_CNT) {
		ctx->trx_num = ctx->trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Waiting for transaction #" << ctx->trx_num);
		
		// ************************************************************************
		// Step 1: Waits for transaction-manager to post a ItemInfoRequest job and sends back ItemInfo 
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_RECEIVE
		DEBUG_COUT ("[Recv] ItemInfoRequest for item " << ctx->item_info_request.I_ID);
		
		memcpy(&ctx->item_info_response.item_version, &TradResManager::items_region[ctx->item_info_request.I_ID], sizeof(ItemVersion));
		
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->lock_req_mr, (uintptr_t)&ctx->lock_request, sizeof(struct LockRequest)));
		TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->item_info_res_mr, (uintptr_t)&ctx->item_info_response, sizeof(struct ItemInfoResponse), false));
		DEBUG_COUT ("[Sent] ItemInfoResponse for item " << ctx->item_info_response.item_version.item.I_ID);
		
		
		// ************************************************************************
		// Step 2: Waits for transaction-manager to post a LockRequest job and responds back
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_RECEIVE
		DEBUG_COUT ("[Recv] Lock request for item " << ctx->lock_request.I_ID);
		
		if (lock_item(ctx->lock_request.I_ID) == false) {
			// unable to acquire lock
			ctx->lock_response.was_successful = false;
			DEBUG_COUT("[Info] Unable to lock item " << ctx->lock_request.I_ID);
		}
		else {
			// lock acquired successfully
			ctx->lock_response.was_successful = true;
			DEBUG_COUT("[Info] Successfully lock item " << ctx->lock_request.I_ID);
			
			// TODO: should be this way: only when the lock got successfully acquired, the RM waits for a follow-up message: 
			// either unlock (in case of abort), or install versions (in case of commit)
		}
		TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->write_data_req_mr, (uintptr_t)&ctx->write_data_request, sizeof(struct WriteDataRequest)));
			
		// Re-arm for the next lock request 
		if (ctx->trx_num < TRANSACTION_CNT)
			TEST_NZ (RDMACommon::post_RECEIVE(ctx->qp, ctx->item_info_req_mr, (uintptr_t)&ctx->item_info_request, sizeof(struct ItemInfoRequest)));
		
		
		
		TEST_NZ (RDMACommon::post_SEND(ctx->qp, ctx->lock_res_mr, (uintptr_t)&ctx->lock_response, sizeof(struct LockResponse), true));
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_SEND (lock_response)
		DEBUG_COUT("[Sent] Lock response to TM");
		

		// ************************************************************************
		// Step 3: Waits for transaction-manager to post a WriteDataRequest job (TODO: should be only when the locking was successful)
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_RECEIVE

		// check the type of the message: either UNLOCK or INSTALL_VERSION
		if (ctx->write_data_request.type == WriteDataRequest::UNLOCK) {
			DEBUG_COUT("[Recv] WriteDataRequest UNLOCK for item " << ctx->write_data_request.content.I_ID);
			TEST_NZ (unlock_item(ctx->write_data_request.content.I_ID));
			DEBUG_COUT("[Info] Successfully unlock item " << ctx->write_data_request.content.I_ID);
		}
		else if (ctx->write_data_request.type == WriteDataRequest::DO_NOTHING)
			DEBUG_COUT("[Recv] WriteDataRequest DO_NOTHING for item " << ctx->write_data_request.content.I_ID);

		else {
			DEBUG_COUT("[Recv] WriteDataRequest INSTALL_VERSION for item " << ctx->write_data_request.content.I_ID);
			
			cart_line = &ctx->write_data_request.content.write_ver_req.cart_line;
			commit_ts = &ctx->write_data_request.content.write_ver_req.commit_ts;
	
			TEST_NZ (decremenet_item_stock(cart_line->SCL_I_ID, cart_line->SCL_QTY, commit_ts));
			unlock_item(cart_line->SCL_I_ID);
			
			DEBUG_COUT("[Info] Decremented stock and unlock for item " << cart_line->SCL_I_ID
				<< "(Q: " << cart_line->SCL_QTY << " TS: " << commit_ts->value << ")");
		}
	}
	
	// close server socket
	std::cout << "[Info] RM-Server is done, now destroying resources!" << std::endl;
	
	TEST_NZ (ctx->destroy_context());
}


int TradResManager::start_server (int server_num) {
	TradResManagerContext ctx[CLIENTS_CNT];
	pthread_t master_threads[CLIENTS_CNT];
	char temp_char;
	
	TradResManager::tcp_port	= TCP_PORT[server_num];
	int ib_port					= IB_PORT[server_num];
		
	TEST_NZ (initialize_data_structures());
	
	TEST_NZ (establish_tcp_connection(TRX_MANAGER_ADDR.c_str(), TRX_MANAGER_TCP_PORT, &TradResManager::res_mng_sockfd));
	
	for (int c = 0; c < CLIENTS_CNT; c++) {
		ctx[c].ib_port	= ib_port;
		
		TEST_NZ (ctx[c].create_context());
		DEBUG_COUT ("[Info] Context created for client #" << c);
		
		// RM posts the RECEIVE job to be ready for the TM's ItemInfoRequest
		TEST_NZ (RDMACommon::post_RECEIVE(ctx[c].qp, ctx[c].item_info_req_mr, (uintptr_t)&ctx[c].item_info_request, sizeof(struct ItemInfoRequest)));
	
		TEST_NZ (RDMACommon::connect_qp (&(ctx[c].qp), ctx[c].ib_port, ctx[c].port_attr.lid, TradResManager::res_mng_sockfd));
		DEBUG_COUT("[Conn] Thread with QP " << ctx->qp->qp_num << " connected to Transaction Manager");
		
		pthread_create(&master_threads[c], NULL, TradResManager::handle_trx_mng_thread, &ctx[c]);
		DEBUG_COUT("[Info] Thread created for a new client");
	}
	std::cout << "[Conn] RM connected to all clients" << std::endl;
	
	for (int c = 0; c < CLIENTS_CNT; c++)
		pthread_join(master_threads[c], NULL);
	
	TEST_NZ (destroy_resources());
}

int main (int argc, char *argv[]) {
	if (argc != 2) {
		TradResManager::usage(argv[0]);
		return 1;
	}
	TradResManager server;
	server.start_server(atoi(argv[1]));
	return 0;
}