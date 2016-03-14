/*
 *	IPTradResManager.hpp
 *
 *	Created on: 13.Feb.2015
 *	Author: Erfan Zamanian
 */


#include "../../two-sided/IPoIB/IPTradResManager.hpp"

#include "../../util/utils.hpp"
#include "../../util/RDMACommon.hpp"
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

void* IPTradResManager::handle_trx_mng_thread(void *param) {
	IPTradResManagerContext *ctx = (IPTradResManagerContext *)param;
	
	ShoppingCartLine	*cart_line;
	Timestamp			*commit_ts;
	
	ctx->trx_num = 0;
	while(ctx->trx_num < TRANSACTION_CNT) {
		ctx->trx_num = ctx->trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Waiting for transaction #" << ctx->trx_num);
		
		// ************************************************************************
		// Step 1: Waits for transaction-manager to post a ItemInfoRequest job and sends back ItemInfo
		TEST_NZ (sock_read(ctx->sockfd, (char *) &ctx->item_info_request, sizeof(struct ItemInfoRequest)));	
		DEBUG_COUT ("[Recv] ItemInfoRequest for item " << ctx->item_info_request.I_ID);
		
		TEST_NZ (sock_write(ctx->sockfd, (char *)&(IPTradResManager::items_region[ctx->item_info_request.I_ID]), sizeof(struct ItemInfoResponse)));	
		DEBUG_COUT ("[Sent] ItemInfoResponse for item " << items_region[ctx->item_info_request.I_ID].item.I_ID);
		
		
		// ************************************************************************
		// Step 2: Waits for transaction-manager to post a LockRequest job and responds back
		TEST_NZ (sock_read(ctx->sockfd, (char *) &(ctx->lock_request), sizeof(struct LockRequest)));
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
		
		TEST_NZ (sock_write(ctx->sockfd, (char *) &(ctx->lock_response), sizeof(struct LockResponse)));	
		DEBUG_COUT("[Sent] Lock response to TM");
		
		// ************************************************************************
		// Step 3: Waits for transaction-manager to post a WriteDataRequest job (TODO: should be only when the locking was successful)
		TEST_NZ (sock_read(ctx->sockfd, (char *) &(ctx->write_data_request), sizeof(struct WriteDataRequest)));	
		
		
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
		
		// Writing the ack to TM 
		ctx->write_data_response.I_ID = ctx->write_data_request.content.I_ID;
		TEST_NZ (sock_write(ctx->sockfd, (char *) &(ctx->write_data_response), sizeof(struct WriteDataResponse)));
	}
	
	// close server socket
	std::cout << "[Info] RM-Server is done, now destroying resources!" << std::endl;
	
	TEST_NZ (ctx->destroy_context());
}


int IPTradResManager::start_server (int server_num) {
	IPTradResManagerContext ctx[CLIENTS_CNT];
	pthread_t master_threads[CLIENTS_CNT];
	char temp_char;
	
	IPTradResManager::tcp_port	= TCP_PORT[server_num];
			
	TEST_NZ (initialize_data_structures());
	
	for (int c = 0; c < CLIENTS_CNT; c++) {
		TEST_NZ (establish_tcp_connection(TRX_MANAGER_ADDR.c_str(), TRX_MANAGER_TCP_PORT, &ctx[c].sockfd));
		DEBUG_COUT("[Comm] Tunnel " << c << " created to TM on sock " << ctx[c].sockfd);
		pthread_create(&master_threads[c], NULL, handle_trx_mng_thread, &ctx[c]);
		DEBUG_COUT("[Info] Thread created for a new client");
	}
	std::cout << "[Conn] RM connected to all clients" << std::endl;
	
	for (int c = 0; c < CLIENTS_CNT; c++)
		pthread_join(master_threads[c], NULL);
	
	TEST_NZ (destroy_resources());
}

int main (int argc, char *argv[]) {
	if (argc != 2) {
		IPTradResManager::usage(argv[0]);
		return 1;
	}
	IPTradResManager server;
	server.start_server(atoi(argv[1]));
	return 0;
}