/*
 *	server.cpp
 *
 *	Created on: 25.01.2015
 *	Author: erfanz
 */

#include "../../config.hpp"
#include "../util/utils.hpp"
#include "server.hpp"
#include "RDMACommon.hpp"
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
using namespace std;


ItemVersion*		Server::items_region			= new ItemVersion[MAX_ITEM_CNT * MAX_ITEM_VERSIONS];
OrdersVersion*		Server::orders_region			= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
OrderLineVersion*	Server::order_line_region		= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
CCXactsVersion*		Server::cc_xacts_region			= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];
TimestampOracle*	Server::timestamp_region		= new TimestampOracle[1];
uint64_t*			Server::lock_items_region		= new uint64_t[MAX_ITEM_CNT];

int* Server::last_orders_cnt	= new int[1];
int* Server::last_cc_xacts_cnt	= new int[1]; 

int Server::server_sockfd = -1;


int Server::create_context(struct Context *ctx)
{	
	TEST_NZ(build_connection(ctx));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx->ib_ctx, ctx->pd, ctx->cq, &(ctx->qp)));
	
	return 0;
}

int Server::build_connection(Context *ctx)
{	
	struct ibv_device **dev_list = NULL;
	struct ibv_device *ib_dev = NULL;
	int cq_size = 0;
	int num_devices;
	
	// get device names in the system
	TEST_Z(dev_list = ibv_get_device_list (&num_devices));
	
	// if there isn't any IB device in host
	TEST_Z(num_devices);
	
	// DEBUG_COUT ("found " << num_devices << " device(s)");
	
	// select the first device
	const char *dev_name = strdup (ibv_get_device_name (dev_list[0]));
	
	// if the device wasn't found in host
	TEST_Z(ib_dev = dev_list[0]);
	
	// get device handle
	TEST_Z(ctx->ib_ctx = ibv_open_device (ib_dev));
	
	
	// We are now done with device list, free it
	ibv_free_device_list (dev_list);
	dev_list = NULL;
	ib_dev = NULL;
	
	// query port properties
	TEST_NZ (ibv_query_port (ctx->ib_ctx, IB_PORT, &ctx->port_attr));
	
	
	// allocate Protection Domain
	TEST_Z(ctx->pd = ibv_alloc_pd (ctx->ib_ctx));
	
	// Create completion channel and completion queue
	TEST_Z(ctx->comp_channel = ibv_create_comp_channel(ctx->ib_ctx));
	cq_size = 10;	// the size of the completion queue
	TEST_Z(ctx->cq = ibv_create_cq (ctx->ib_ctx, cq_size, NULL, ctx->comp_channel, 0));
	return 0;
}

int Server::register_memory(Context *ctx)
{
	int mr_flags = 0;
	int i_s;
	int o_s;
	int ol_s;
	int cc_s;
	int ts_s;
	int lock_item_s;
	
	ctx->send_msg = new Message[1];

	i_s			= MAX_ITEM_CNT * MAX_ITEM_VERSIONS * sizeof(ItemVersion);
	o_s			= MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS * sizeof(OrdersVersion);
	ol_s		= ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS * sizeof(OrderLineVersion);
	cc_s		= MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS * sizeof(CCXactsVersion);
	ts_s		= sizeof(TimestampOracle);
	lock_item_s	= MAX_ITEM_CNT * sizeof(uint64_t);
	
	mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
	
	TEST_Z(ctx->send_mr			= ibv_reg_mr(ctx->pd, ctx->send_msg, sizeof(struct Message), mr_flags));
	TEST_Z(ctx->mr_items		= ibv_reg_mr(ctx->pd, items_region, i_s, mr_flags));
	TEST_Z(ctx->mr_orders		= ibv_reg_mr(ctx->pd, orders_region, o_s, mr_flags));
	TEST_Z(ctx->mr_order_line	= ibv_reg_mr(ctx->pd, order_line_region, ol_s, mr_flags));
	TEST_Z(ctx->mr_cc_xacts		= ibv_reg_mr(ctx->pd, cc_xacts_region, cc_s, mr_flags));
	TEST_Z(ctx->mr_timestamp	= ibv_reg_mr(ctx->pd, timestamp_region, ts_s, mr_flags));
	TEST_Z(ctx->mr_lock_items	= ibv_reg_mr(ctx->pd, lock_items_region, lock_item_s, mr_flags));
	
	return 0;
}


void* Server::handle_client(void *param)
{
	int client_sock = *((int*) param);
	char temp_char;
	
	// init all of the resources, so cleanup will be easy
	struct Context ctx;
	memset (&ctx, 0, sizeof ctx);
	ctx.client_sockfd = client_sock;
	
	// create all resources
	TEST_NZ (create_context(&ctx));
	
	// connect the QPs
	TEST_NZ (connect_qp (&ctx));
	
	// setup server buffer with read message
	memcpy(&(ctx.send_msg->mr_items),		ctx.mr_items,		sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg->mr_orders),		ctx.mr_orders,		sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg->mr_order_line),	ctx.mr_order_line,	sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg->mr_cc_xacts),	ctx.mr_cc_xacts,	sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg->mr_timestamp),	ctx.mr_timestamp,	sizeof(struct ibv_mr));
	memcpy(&(ctx.send_msg->mr_lock_items),	ctx.mr_lock_items,	sizeof(struct ibv_mr));
	
	// send memory locations using SEND 
	TEST_NZ (RDMACommon::post_SEND (ctx.qp, ctx.send_mr, (uintptr_t)ctx.send_msg, sizeof(struct Message)));
	TEST_NZ (RDMACommon::poll_completion(ctx.cq));
	
	/*
		Server waits for the client to muck with its memory
	*/
	
	/* Sync so server will know that client is done mucking with its memory */
	TEST_NZ (sock_sync_data (ctx.client_sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	cout << "final value of the timestamp buffer " << Server::timestamp_region[0].timestamp << endl;
	cout << "final value of lock  " << Lock::get_lock_status(lock_items_region[0]) << " | " << Lock::get_version(lock_items_region[0]) << endl;
	
	TEST_NZ (destroy_context(&ctx));
	return NULL;
}

int Server::connect_qp (struct Context *ctx)
{
	struct CommunicationExchangeData local_con_data, remote_con_data, tmp_con_data;
	char temp_char;
	union ibv_gid my_gid;
	
	memset (&my_gid, 0, sizeof my_gid);
	
	// exchange using TCP sockets info required to connect QPs
	local_con_data.qp_num = htonl (ctx->qp->qp_num);
	local_con_data.lid = htons (ctx->port_attr.lid);
	
	memcpy (local_con_data.gid, &my_gid, sizeof my_gid);
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

int Server::destroy_context (struct Context *ctx)
{
	if (ctx->qp)
		TEST_NZ(ibv_destroy_qp (ctx->qp));
	
	if (ctx->send_mr)
		TEST_NZ (ibv_dereg_mr (ctx->send_mr));
	
	if (ctx->mr_items)
		TEST_NZ (ibv_dereg_mr (ctx->mr_items));
	
	if (ctx->mr_orders)
		TEST_NZ (ibv_dereg_mr (ctx->mr_orders));
	
	if (ctx->mr_order_line)
		TEST_NZ (ibv_dereg_mr (ctx->mr_order_line));
	
	if (ctx->mr_cc_xacts)
		TEST_NZ (ibv_dereg_mr (ctx->mr_cc_xacts));
	
	if (ctx->mr_timestamp)
		TEST_NZ (ibv_dereg_mr (ctx->mr_timestamp));
	
	if (ctx->mr_lock_items)
		TEST_NZ (ibv_dereg_mr (ctx->mr_lock_items));
	
	delete[](ctx->send_msg);
	
	if (ctx->cq)
		TEST_NZ (ibv_destroy_cq (ctx->cq));
	
	if (ctx->pd)
		TEST_NZ (ibv_dealloc_pd (ctx->pd));
	
	if (ctx->ib_ctx)
		TEST_NZ (ibv_close_device (ctx->ib_ctx));

	if (ctx->client_sockfd >= 0){
		TEST_NZ (close (ctx->client_sockfd));
	}
}

int Server::destroy_resources ()
{
	delete[](Server::items_region);
	delete[](Server::orders_region);
	delete[](Server::order_line_region);
	delete[](Server::cc_xacts_region);
	delete[](Server::timestamp_region);
	delete[](Server::lock_items_region);
	
	close(Server::server_sockfd);	// close the socket
	return 0;
}

int Server::initialize_data_structures(){
	
	Server::timestamp_region[0].timestamp = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("Timestamp set to " << Server::timestamp_region[0].timestamp);

	TEST_NZ(load_tables_from_files());
	DEBUG_COUT("tables loaded successfully");
	
	int i;
	uint32_t stat;
	uint32_t version;
	for (i=0; i < MAX_ITEM_CNT; i++)
	{
		stat = (uint32_t)0;	// 0 for free, 1 for locked
		version = (uint32_t)0;	// the first version of each item is 0
		lock_items_region[i] = Lock::set_lock(stat, version);
	}
	DEBUG_COUT("All locks set free");
		
	Server::last_orders_cnt[0] = 0;
	Server::last_cc_xacts_cnt[0] = 0;
	return 0;
}

int Server::load_tables_from_files() {
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	
	// First load ITEM file
	try{
		fp = fopen(ITEM_FILENAME, "r");
		if (fp == NULL){
			cerr << "Cannot open file: " << ITEM_FILENAME << endl;
			exit(EXIT_FAILURE);
		}		
		int i = 0;
		
		while ((read = getline(&line, &len, fp)) != -1) {
			if (i >= MAX_ITEM_CNT)
				// we don't want to read more data from file.
				break;
			
			Server::items_region[i].item.I_ID = atoi(strtok(line,  "\t"));
			strcpy(Server::items_region[i].item.I_TITLE,strtok(NULL, "\t"));
			Server::items_region[i].item.I_A_ID = atoi(strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_PUB_DATE, strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_PUBLISHER, strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_SUBJECT, strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_DESC, strtok(NULL, "\t"));
			Server::items_region[i].item.I_RELATED1 = atoi(strtok(NULL, "\t"));
			Server::items_region[i].item.I_RELATED2 = atoi(strtok(NULL, "\t"));
			Server::items_region[i].item.I_RELATED3 = atoi(strtok(NULL, "\t"));
			Server::items_region[i].item.I_RELATED4 = atoi(strtok(NULL, "\t"));
			Server::items_region[i].item.I_RELATED5 = atoi(strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_THUMBNAIL, strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_IMAGE, strtok(NULL, "\t"));
			Server::items_region[i].item.I_SRP = atof(strtok(NULL, "\t"));
			Server::items_region[i].item.I_COST = atof(strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_AVAIL, strtok(NULL, "\t"));
			Server::items_region[i].item.I_STOCK = atoi(strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_ISBN, strtok(NULL, "\t"));
			Server::items_region[i].item.I_PAGE = atoi(strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_BACKING, strtok(NULL, "\t"));
			strcpy(Server::items_region[i].item.I_DIMENSION, strtok(NULL, "\t"));
			
			Server::items_region[i].write_timestamp = 0;	
			i++;
		}

		fclose(fp);
		
	}
	catch (std::exception& e){
	    std::cerr << "exception caught: " << e.what() << '\n';
	}	
	if (line)
		free(line);
	
	return 0;
}

void Server::die(const char *reason)
{
	fprintf(stderr, "%s\n", reason);
	fprintf(stderr, "Errno: %s\n", strerror(errno));	
	exit(EXIT_FAILURE);
}

void Server::usage (const char *argv0) {
	cout << "Usage:" << endl;
	cout << argv0 << " starts a server and wait for connection" << endl;
}

int Server::start_server ()
{	
	TEST_NZ(initialize_data_structures());
	
	DEBUG_COUT("waiting for " << CLIENTS_CNT << " client(s) on port " << TCP_PORT << " for TCP connection!");
	
	Server::server_sockfd = -1;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	pthread_t master_threads[CLIENTS_CNT];	
	int client_socks[CLIENTS_CNT];
	
	// Open Socket
	Server::server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (Server::server_sockfd < 0)
	{
		cerr << "Error opening socket" << endl;
		return -1;
	}
	
	// Bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TCP_PORT);
	TEST_NZ(bind(Server::server_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)));
	
	
	// listen				  
	TEST_NZ(listen (Server::server_sockfd, CLIENTS_CNT));
	
	// accept connections
	clilen = sizeof(cli_addr);
	int i = 0;
	while (i < CLIENTS_CNT){
		client_socks[i]  = accept (Server::server_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (client_socks[i] < 0){ 
			cerr << "ERROR on accept" << endl;
			return -1;
		}
		DEBUG_COUT("received client #" << i);
		pthread_create(&master_threads[i], NULL, Server::handle_client, &client_socks[i]);
		i++;
	}
	
	//wait for handlers to finish
	for (i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	
	// close server socket
	DEBUG_COUT ("Server is done, now destroying resources!");
	TEST_NZ(destroy_resources());
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
******************************************************************************/
int main (int argc, char *argv[])
{
	if (argc != 1) {
		Server::usage(argv[0]);
		return 1;
	}
	
	Server server;
	server.start_server();
	
	return 0;
}