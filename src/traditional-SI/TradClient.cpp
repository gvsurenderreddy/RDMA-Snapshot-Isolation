/*
 *	TradClient.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "TradClient.hpp"
#include "../../config.hpp"
#include "../util/utils.hpp"
#include "../util/RDMACommon.hpp"
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


void TradClient::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to the transaction_manager server specified in Config.TRX_MANAGER_ADDR" << std::endl;
}

void TradClient::fill_shopping_cart(struct Cart *cart) {
	int item_id;
	int quantity;
	
	DEBUG_COUT ("[Info] Contents of the cart: (Item ID,  Quantity)");
	for (int i=0; i < ORDERLINE_PER_ORDER; i++) {
		item_id		= (i * ITEM_PER_SERVER) + (rand() % ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		quantity	= (rand() % 5) + 1;			// the quanity of the item (not importatn)
		cart->cart_lines[i].SCL_I_ID	= item_id;
		cart->cart_lines[i].SCL_QTY		= quantity;
		DEBUG_COUT (".... " <<  item_id << '\t' << quantity);
	}
}

int TradClient::start_transaction(TradClientContext &ctx) {
	int abort_cnt = 0;
	struct timespec firstRequestTime, lastRequestTime; // for calculating TPMS
	char temp_char;
	
	// Final handshake with the server before starting the transactions, to ensure server is ready!
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "T", &temp_char));	// just send a dummy char back and forth
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	
	ctx.trx_num = 0;
	while (ctx.trx_num  <  TRANSACTION_CNT) {
		ctx.trx_num = ctx.trx_num + 1;
		DEBUG_COUT (std::endl << "[Info] Submitting transaction #" << ctx.trx_num);
		
		/*
		// ************************************************************************
		//	Clients sends ItemInfoRequest to receive info on the item
		item_id = rand() % MAX_ITEM_CNT;	// generating in the range 0 to MAX_ITEM_CNT
		ctx->item_info_request->I_ID = item_id;
		
		TEST_NZ (RDMACommon::post_RECEIVE (ctx->qp, ctx->item_info_res_mr, (uintptr_t)ctx->item_info_response, sizeof(struct ItemInfoResponse)));
		TEST_NZ (RDMACommon::post_SEND (ctx->qp, ctx->item_info_req_mr, (uintptr_t)ctx->item_info_request, sizeof(struct ItemInfoRequest)));
		TEST_NZ (RDMACommon::poll_completion (ctx->cq));		// completion for post_SEND
		DEBUG_COUT("Successfully sent ItemInfoRequest to server for item " << item_id);
		
		
		// ************************************************************************
		//	Clients waits for ItemInfoResponse which contains information about the item
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));		// completion for post_RECEIVE
		
		// currently, there is nothing special in CommitRequest that the user has to fill in.  
		DEBUG_COUT("Received ItemInfoResponse for item " << ctx->item_info_response->item.I_ID);
		*/
		
		
		// ************************************************************************
		//	Client fills their shopping cart
		fill_shopping_cart(&ctx.commit_request.cart);
			
		
		// ************************************************************************
		//	Client requests for commit
		TEST_NZ (RDMACommon::post_RECEIVE (ctx.qp, ctx.commit_res_mr, (uintptr_t)&ctx.commit_response, sizeof(struct CommitResponse)));
		TEST_NZ (RDMACommon::post_SEND (ctx.qp, ctx.commit_req_mr, (uintptr_t)&ctx.commit_request, sizeof(struct CommitRequest), true));
		TEST_NZ (RDMACommon::poll_completion (ctx.cq));		// completion for post_SEND
		DEBUG_COUT("[Sent] CommitRequest to TM");
		
		// ************************************************************************
		//	Clients waits for the outcome of the commit
		TEST_NZ (RDMACommon::poll_completion (ctx.cq));		// completion for post_RECEIVE
		if (ctx.commit_response.commit_outcome == CommitResponse::COMMITTED)
			DEBUG_COUT("[Recv] CommitResponse (result: committed)");
		else {
			DEBUG_COUT("[Recv] CommitResponse (result: aborted)");
			abort_cnt++;
		}		
	}

	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	double T_P_MILISEC        = (double)(TRANSACTION_CNT / (double)(micro_elapsed_time / 1000));
	int committed_cnt         = TRANSACTION_CNT - abort_cnt;
	double success_rate       = (double)committed_cnt /  TRANSACTION_CNT;
	
	std::cout << std::endl;
	std::cout << "[Stat] Transaction per millisec:	" <<  T_P_MILISEC << std::endl;
	std::cout << "[Stat] " << committed_cnt << " committed, " << abort_cnt << " aborted (success rate = " << success_rate << ")." << std::endl;
	
	return 0;
}

int TradClient::start_client () {	
	// Init Context
	TradClientContext ctx;
	char temp_char;
	
	ctx.ib_port = TRX_MANAGER_IB_PORT;
	
	TEST_NZ (establish_tcp_connection(TRX_MANAGER_ADDR.c_str(), TRX_MANAGER_TCP_PORT, &ctx.sockfd));
	
	TEST_NZ (ctx.create_context());
	TEST_NZ (RDMACommon::connect_qp (&(ctx.qp), ctx.ib_port, ctx.port_attr.lid, ctx.sockfd));	
	
	srand (time(NULL));		// initialize random seed
	
	start_transaction(ctx);
	
	/* Sync so server will know that client is done mucking with its memory */
	DEBUG_COUT("[Info] Client is done, and is ready to destroy its resources!");
	TEST_NZ (sock_sync_data (ctx.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	TEST_NZ(ctx.destroy_context());
}

/******************************************************************************
* Function: main
*
* Input
* argc number of items in argv
* argv command line parameters
*
* Output
* none
*
* Returns
* 0 on success, 1 on failure
*
* Description
* Main program code
******************************************************************************/
int main (int argc, char *argv[]) {
	/* parse the command line parameters */
	if (argc != 1) {
		TradClient::usage(argv[0]);
		return 1;
	}
	TradClient client;
	client.start_client();
	return 0;
}
