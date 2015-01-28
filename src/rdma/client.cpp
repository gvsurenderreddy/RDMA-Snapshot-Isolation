/*
 *	client.cpp
 *
 *	Created on: 25.01.2015
 *	Author: erfanz
 */

#include "../../config.hpp"
#include "../util/utils.hpp"
#include "client.hpp"
#include "RDMACommon.hpp"
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
	DEBUG_COUT("searching for IB devices in host");
	
	// get device names in the system
	TEST_Z(dev_list = ibv_get_device_list (&num_devices));
	
	TEST_Z(num_devices); // if there isn't any IB device in host
	
	DEBUG_COUT ("found " << num_devices << " device(s)");
	
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
	cq_size = 10;	// the size of the completion queue
	TEST_Z(ctx->cq = ibv_create_cq (ctx->ib_ctx, cq_size, NULL, ctx->comp_channel, 0));
	
	return 0;
}


int Client::register_memory(Context *ctx)
{
	int mr_flags = 0;
	int items_size;
	int orders_size;
	int cc_xacts_size;
	int ts_size;
	
	ctx->recv_msg = new Message[1];
	
	ctx->local_rdma_items_region		= new ItemVersion[FETCH_BLOCK_SIZE];
	ctx->local_rdma_orders_region		= new OrdersVersion[1];
	ctx->local_rdma_cc_xacts_region		= new CCXactsVersion[1];
	ctx->local_rdma_timestamp_region	= new TimestampOracle[3];	// first element: read timestamp,  second element: commit timestamp
	
	items_size		= FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	orders_size		= sizeof(OrdersVersion);
	cc_xacts_size	= sizeof(CCXactsVersion);
	ts_size			= 3 * sizeof(TimestampOracle);		// one for read timestamp, one for commit timestamp
	
	mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	TEST_Z(ctx->recv_mr							= ibv_reg_mr(ctx->pd, ctx->recv_msg, sizeof(struct Message), mr_flags));
	TEST_Z(ctx->local_rdma_items_mr				= ibv_reg_mr(ctx->pd, ctx->local_rdma_items_region, items_size, mr_flags));
	TEST_Z(ctx->local_rdma_orders_mr			= ibv_reg_mr(ctx->pd, ctx->local_rdma_orders_region, orders_size, mr_flags));
	TEST_Z(ctx->local_rdma_cc_xacts_mr			= ibv_reg_mr(ctx->pd, ctx->local_rdma_cc_xacts_region, cc_xacts_size, mr_flags));
	TEST_Z(ctx->local_rdma_timestamp_oracle_mr	= ibv_reg_mr(ctx->pd, ctx->local_rdma_timestamp_region, ts_size, mr_flags));
	
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
	fprintf (stdout, "\nLocal LID = 0x%x\n", ctx->port_attr.lid);
	
	TEST_NZ (sock_sync_data(ctx->client_sockfd, sizeof (struct CommunicationExchangeData), (char *) &local_con_data, (char *) &tmp_con_data));
	
	remote_con_data.qp_num = ntohl (tmp_con_data.qp_num);
	remote_con_data.lid = ntohs (tmp_con_data.lid);
	memcpy (remote_con_data.gid, tmp_con_data.gid, 16);
	
	// save the remote side attributes, we will need it for the post SR
	ctx->remote_props = remote_con_data;
	fprintf (stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
	fprintf (stdout, "Remote LID = 0x%x\n", remote_con_data.lid);
	
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
	
	if (ctx->recv_mr)
		TEST_NZ (ibv_dereg_mr (ctx->recv_mr));
	
	if (ctx->local_rdma_items_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_rdma_items_mr));
			
	if (ctx->local_rdma_orders_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_rdma_orders_mr));
	
	if (ctx->local_rdma_cc_xacts_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_rdma_cc_xacts_mr));
	
	if (ctx->local_rdma_timestamp_oracle_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_rdma_timestamp_oracle_mr));
	
	delete[](ctx->recv_msg);
	delete[](ctx->local_rdma_items_region);
	delete[](ctx->local_rdma_orders_region);
	delete[](ctx->local_rdma_cc_xacts_region);
	delete[](ctx->local_rdma_timestamp_region);
	
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
	fprintf(stderr, "%s\n", reason);
	fprintf(stderr, "Errno: %s\n", strerror(errno));	
	exit(EXIT_FAILURE);
}

void Client::usage (const char *argv0)
{
	cout << "Usage:" << endl;
	cout << argv0 << " <host> connects to server at <host>" << endl;
}

int Client::start_client ()
{	
	// Init Context
	struct Context ctx;
	char temp_char;
	
    memset (&ctx, 0, sizeof ctx);
    ctx.client_sockfd = -1;
	
	TEST_NZ (create_context (&ctx));
	
	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(ctx.qp, ctx.recv_mr, (uintptr_t)ctx.recv_msg, sizeof(struct Message));
	
	TEST_NZ(connect_qp (&ctx));
	
	TEST_NZ(RDMACommon::poll_completion(ctx.cq));
	
	fprintf (stdout, "MR was registered with lkey=0x%x, rkey=0x%x\n",
	        ctx.recv_msg->rdma_mr_items.lkey, ctx.recv_msg->rdma_mr_items.rkey);
	
	
	/*
	
	// Sync so we are sure server side has data ready before client tries to read it
	if (sock_sync_data (ctx.sock, 1, "R", &temp_char))	// just send a dummy char back and forth
		die("sync error before RDMA ops");
		
	
	// Now the client performs an RDMA read and then write on server.
	// Note that the server has no idea these events have occured
	
	// First we read contens of server's buffer
	if (post_send (&ctx, IBV_WR_RDMA_READ))
		die("failed to post SR 2");

	
	if (poll_completion (&ctx))
		die("poll completion failed 2");
	
	//fprintf (stdout, "Contents of server's buffer: '%s'\n", ctx.buf);
	
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