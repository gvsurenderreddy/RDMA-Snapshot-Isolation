/*
 *	rdma-client.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "traditional-client.hpp"
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
#include <time.h>	// for struct timespec


char* Client::server_name = NULL;

int Client::create_context (struct Context *ctx)
{
	TEST_NZ(build_connection(ctx));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx->ib_ctx, ctx->pd, ctx->cq, &(ctx->qp)));
	
	return 0;
}

int Client::build_connection(Context *ctx)
{
	struct ibv_device **dev_list = NULL;
	struct ibv_device *ib_dev = NULL;
	int cq_size = 0;
	int num_devices;
	
	ctx->client_sockfd = sock_connect (Client::server_name, TCP_PORT);
	if (ctx->client_sockfd < 0)
	{
		cerr << "failed to establish TCP connection to server " <<  Client::server_name << ", port " << TCP_PORT << endl;
		destroy_context(ctx);
		return -1;
	}
	
	DEBUG_COUT("TCP connection was established");
	// DEBUG_COUT("searching for IB devices in host");
	
	// get device names in the system
	TEST_Z(dev_list = ibv_get_device_list (&num_devices));
	
	TEST_Z(num_devices); // if there isn't any IB device in host
	
	// DEBUG_COUT ("found " << num_devices << " device(s)");
	
	// select the first device
	const char *dev_name = strdup (ibv_get_device_name (dev_list[0]));
	
	TEST_Z(ib_dev = dev_list[0]);	// if the device wasn't found in host
	
	TEST_Z(ctx->ib_ctx = ibv_open_device (ib_dev));		// get device handle
	
	// We are now done with device list, free it
	ibv_free_device_list (dev_list);
	dev_list = NULL;
	ib_dev = NULL;
	
	TEST_NZ (ibv_query_port (ctx->ib_ctx, IB_PORT, &ctx->port_attr));	// query port properties
	
	TEST_Z(ctx->pd = ibv_alloc_pd (ctx->ib_ctx));		// allocate Protection Domain
	
	// Create completion channel and completion queue
	TEST_Z(ctx->comp_channel = ibv_create_comp_channel(ctx->ib_ctx));
	cq_size = 1000;	// the size of the completion queue
	TEST_Z(ctx->cq = ibv_create_cq (ctx->ib_ctx, cq_size, NULL, ctx->comp_channel, 0));
	
	return 0;
}


int Client::register_memory(Context *ctx)
{
	int mr_flags = 0;
	ctx->item_info_request	= new ItemInfoRequest[1];
	ctx->item_info_response	= new ItemInfoResponse[1];
	ctx->commit_request		= new CommitRequest[1];
	ctx->commit_response	= new CommitResponse[1];
	
	mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	TEST_Z(ctx->item_info_req_mr	= ibv_reg_mr(ctx->pd, ctx->item_info_request, sizeof(struct ItemInfoRequest), mr_flags));
	TEST_Z(ctx->item_info_res_mr	= ibv_reg_mr(ctx->pd, ctx->item_info_response, sizeof(struct ItemInfoResponse), mr_flags));
	TEST_Z(ctx->commit_req_mr		= ibv_reg_mr(ctx->pd, ctx->commit_request, sizeof(struct CommitRequest), mr_flags));
	TEST_Z(ctx->commit_res_mr		= ibv_reg_mr(ctx->pd, ctx->commit_response, sizeof(struct CommitResponse), mr_flags));
	
	return 0;
}

int Client::connect_qp (struct Context *ctx)
{
	struct CommunicationExchangeData local_con_data, remote_con_data, tmp_con_data;
	char temp_char;
	union ibv_gid my_gid;
	
	memset (&my_gid, 0, sizeof my_gid);
	
	/* exchange using TCP sockets info required to connect QPs */
	local_con_data.qp_num = htonl (ctx->qp->qp_num);
	local_con_data.lid = htons (ctx->port_attr.lid);
	
	memcpy (local_con_data.gid, &my_gid, 16);
	// fprintf (stdout, "\nLocal LID = 0x%x\n", ctx->port_attr.lid);
	
	TEST_NZ (sock_sync_data(ctx->client_sockfd, sizeof (struct CommunicationExchangeData), (char *) &local_con_data, (char *) &tmp_con_data));
	
	remote_con_data.qp_num = ntohl (tmp_con_data.qp_num);
	remote_con_data.lid = ntohs (tmp_con_data.lid);
	memcpy (remote_con_data.gid, tmp_con_data.gid, 16);
	
	// save the remote side attributes, we will need it for the post SR
	ctx->remote_props = remote_con_data;
	// fprintf (stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
	// fprintf (stdout, "Remote LID = 0x%x\n", remote_con_data.lid);
	
	// modify the QP to init
	TEST_NZ(RDMACommon::modify_qp_to_init (IB_PORT, ctx->qp));
	
	// modify the QP to RTR
	TEST_NZ(RDMACommon::modify_qp_to_rtr (IB_PORT, ctx->qp, remote_con_data.qp_num, remote_con_data.lid, remote_con_data.gid));
	
	// modify the QP to RTS
	TEST_NZ(RDMACommon::modify_qp_to_rts (ctx->qp));
	
	// sync to make sure that both sides are in states that they can connect to prevent packet loss
	TEST_NZ(sock_sync_data (ctx->client_sockfd, 1, "Q", &temp_char));	// just send a dummy char back and forth
	
	return 0;
}

int Client::destroy_context (struct Context *ctx)
{
	if (ctx->qp)
		TEST_NZ(ibv_destroy_qp (ctx->qp));
	
	if (ctx->item_info_req_mr)
		TEST_NZ (ibv_dereg_mr (ctx->item_info_req_mr));
	
	if (ctx->item_info_res_mr)
		TEST_NZ (ibv_dereg_mr (ctx->item_info_res_mr));
	
	if (ctx->commit_req_mr)
		TEST_NZ (ibv_dereg_mr (ctx->commit_req_mr));
	
	if (ctx->commit_res_mr)
		TEST_NZ (ibv_dereg_mr (ctx->commit_res_mr));
	
	delete[](ctx->item_info_request);
	delete[](ctx->item_info_response);
	delete[](ctx->commit_request);
	delete[](ctx->commit_response);
	
	if (ctx->cq)
		TEST_NZ (ibv_destroy_cq (ctx->cq));
	
	if (ctx->pd)
		TEST_NZ (ibv_dealloc_pd (ctx->pd));
	
	if (ctx->ib_ctx)
		TEST_NZ (ibv_close_device (ctx->ib_ctx));

	if (ctx->client_sockfd >= 0)
		TEST_NZ (close (ctx->client_sockfd));
	
	return 0;
}

void Client::die(const char *reason)
{
	cerr <<  reason << endl;
	cerr << "Errno: " << strerror(errno) << endl;	
	exit(EXIT_FAILURE);
}

void Client::usage (const char *argv0)
{
	cout << "Usage:" << endl;
	cout << argv0 << " <host> connects to server at <host>" << endl;
}

int Client::start_transaction(Context *ctx)
{
	int item_id;
	int abort_cnt = 0;
	struct timespec firstRequestTime, lastRequestTime; // for calculating TPMS
	ctx->transaction_statement_number = 0;
		
	DEBUG_COUT ("Transaction now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	
	while (ctx->transaction_statement_number  <  TRANSACTION_CNT){
		ctx->transaction_statement_number = ctx->transaction_statement_number + 1;
		DEBUG_COUT (endl << "Handling transaction #" << ctx->transaction_statement_number);
		
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
		
		
		// ************************************************************************
		//	Clients requests for commit
		TEST_NZ (RDMACommon::post_RECEIVE (ctx->qp, ctx->commit_res_mr, (uintptr_t)ctx->commit_response, sizeof(struct CommitResponse)));
		TEST_NZ (RDMACommon::post_SEND (ctx->qp, ctx->commit_req_mr, (uintptr_t)ctx->commit_request, sizeof(struct CommitRequest)));
		TEST_NZ (RDMACommon::poll_completion (ctx->cq));		// completion for post_SEND
		DEBUG_COUT("Successfully sent CommitRequest to server");
		
		
		// ************************************************************************
		//	Clients waits for the outcome of the commit
		TEST_NZ (RDMACommon::poll_completion (ctx->cq));		// completion for post_RECEIVE
		if (ctx->commit_response->commit_outcome == CommitResponse::COMMITTED)
			DEBUG_COUT("Received CommitResponse, the transactions is committed!");
		else {
			DEBUG_COUT("Received CommitResponse, the transactions is aborted!");
			abort_cnt++;
		}
	}
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	
	double nano_elapsed_time = ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec );
	double T_P_MILISEC = (double)(TRANSACTION_CNT / (double)(nano_elapsed_time / 1000000));
	cout << "Transaction per millisec: " <<  T_P_MILISEC << endl;
	
	int committed_cnt = TRANSACTION_CNT - abort_cnt;
	double success_rate = (double)committed_cnt /  TRANSACTION_CNT;
	cout << committed_cnt << " committed, " << abort_cnt << " aborted (success rate = " << success_rate << ")." << endl;
	
	return 0;
}

int Client::start_client ()
{	
	// Init Context
	struct Context ctx;
	char temp_char;
	
    memset (&ctx, 0, sizeof ctx);
    ctx.client_sockfd = -1;
	
	TEST_NZ (create_context (&ctx));
	TEST_NZ(connect_qp (&ctx));
	
	srand (time(NULL));		// initialize random seed
	
	start_transaction(&ctx);
	
	/* Sync so server will know that client is done mucking with its memory */
	DEBUG_COUT("Client is done, and is ready to destroy its resources!");
	TEST_NZ (sock_sync_data (ctx.client_sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	TEST_NZ(destroy_context(&ctx));
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
int main (int argc, char *argv[])
{
	/* parse the command line parameters */
	if (argc != 2) {
		Client::usage(argv[0]);
		return 1;
	}
	Client::server_name = argv[1];
	
	Client client;
	client.start_client();
	return 0;
	
}