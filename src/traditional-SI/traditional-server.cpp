/*
 *	traditional-server.cpp
 *
 *	Created on: 9.Feb.2015
 *	Author: erfanz
 */

#include "traditional-server.hpp"
#include "../util/utils.hpp"
#include "../util/RDMACommon.hpp"
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


ItemVersion*		Server::items_region			= new ItemVersion[MAX_ITEM_CNT * MAX_ITEM_VERSIONS];
OrdersVersion*		Server::orders_region			= new OrdersVersion[MAX_ORDERS_CNT * MAX_ORDERS_VERSIONS];
OrderLineVersion*	Server::order_line_region		= new OrderLineVersion[ORDERLINE_PER_ORDER * MAX_ORDERS_CNT * MAX_ORDERLINE_VERSIONS];
CCXactsVersion*		Server::cc_xacts_region			= new CCXactsVersion[MAX_CCXACTS_CNT * MAX_CCXACTS_VERSIONS];
TimestampOracle		Server::timestamp_region;
std::mutex 			Server::timestamp_mutex;
std::mutex	 		Server::item_lock[MAX_ITEM_CNT];
int					Server::server_sockfd			= -1;


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


void* Server::handle_client(void *param)
{
	int client_sock = *((int*) param);
	char temp_char;
	int transaction_statement_number = 0;
	int requested_item_id;
	
	TimestampOracle read_timestamp;
	TimestampOracle commit_timestamp;
	
	int new_order_index;
	int new_orderline_index;
	int new_ccxacts_index;
	int i;	// for for-loops
	
	// init all of the resources, so cleanup will be easy
	struct Context ctx;
	memset (&ctx, 0, sizeof ctx);
	ctx.client_sockfd = client_sock;
	
	// create all resources
	TEST_NZ (create_context(&ctx));
	
	// before connecting the queue pairs, we post the RECEIVE job to be
	// ready for the client's message containing its first request
	TEST_NZ (RDMACommon::post_RECEIVE(ctx.qp, ctx.item_info_req_mr, (uintptr_t)ctx.item_info_request, sizeof(struct ItemInfoRequest)));
	
	// connect the QPs
	TEST_NZ (connect_qp (&ctx));

	while (ctx.transaction_statement_number  <  TRANSACTION_CNT)
	{
		ctx.transaction_statement_number = ctx.transaction_statement_number + 1;
		DEBUG_COUT (std::endl << "Waiting for transaction #" << ctx.transaction_statement_number);
		
		// ************************************************************************
		// Waits for user to post a ItemInfoRequest job
		TEST_NZ (RDMACommon::poll_completion(ctx.cq));		// completion for post_RECEIVE
		memcpy(&read_timestamp, &(Server::timestamp_region), sizeof(TimestampOracle));
		memcpy(&requested_item_id, &(ctx.item_info_request->I_ID), sizeof(ctx.item_info_request->I_ID));
		DEBUG_COUT("Received request for item #" << requested_item_id);

		
		// ************************************************************************
		// Server now fethces ItemInfo and fills in ItemInfoResponse
		memcpy(&(ctx.item_info_response->item), &(items_region[requested_item_id].item), sizeof(Item));
		
		TEST_NZ (RDMACommon::post_RECEIVE(ctx.qp, ctx.commit_req_mr, (uintptr_t)ctx.commit_request, sizeof(struct CommitRequest)));
		TEST_NZ (RDMACommon::post_SEND (ctx.qp, ctx.item_info_res_mr, (uintptr_t)ctx.item_info_response, sizeof(struct ItemInfoResponse)));
		TEST_NZ (RDMACommon::poll_completion(ctx.cq));		// completion for post_SEND
		
		DEBUG_COUT("Successfully sent ItemInfoResponse to client, with read_timestamp " << read_timestamp.timestamp); 
		
		
		// ************************************************************************
		// Server now waits for user to post CommitRequest Job
		TEST_NZ (RDMACommon::poll_completion(ctx.cq));		// completion for psot_RECEIVE
		DEBUG_COUT("Received CommitRequest.");

		
		// ************************************************************************
		//	Commit starts. first acquire locks, and finally fills in CommitResponse
		
		// Get commit timestamp
		timestamp_mutex.lock();
		timestamp_region.timestamp++;
		memcpy(&commit_timestamp, &(Server::timestamp_region), sizeof(TimestampOracle));
		DEBUG_COUT("Acquired commit timestamp " << commit_timestamp.timestamp);
		timestamp_mutex.unlock();
		
		// Lock item and decrement the stock
		item_lock[requested_item_id].lock();
		DEBUG_COUT("Acquired lock on item " << requested_item_id);
		
		// Decrement the stock 
		if (items_region[requested_item_id].item.I_STOCK < 10)
			items_region[requested_item_id].item.I_STOCK += 20;
		else
			items_region[requested_item_id].item.I_STOCK -= 1;
		DEBUG_COUT("Decremented the stock in table ITEM");
		
		
		// Insert the order
		new_order_index = commit_timestamp.timestamp - 1;
		memcpy(&orders_region[new_order_index].orders, &(ctx.commit_request->order), sizeof(Orders));
		orders_region[new_order_index].write_timestamp	= commit_timestamp.timestamp;
		orders_region[new_order_index].orders.O_ID		= commit_timestamp.timestamp;
		DEBUG_COUT("Added a record to table ORDERS");
		
		// Insert orderlines
		new_orderline_index	= ORDERLINE_PER_ORDER * (commit_timestamp.timestamp - 1);
		for (i = 0; i < ORDERLINE_PER_ORDER; i++)
		{
			memcpy(&order_line_region[new_orderline_index + i].order_line, &ctx.commit_request->orderlines[i], sizeof(OrderLine));
			order_line_region[new_orderline_index + i].write_timestamp		= commit_timestamp.timestamp;
			order_line_region[new_orderline_index + i].order_line.OL_ID		= new_orderline_index + i + 1;
			order_line_region[new_orderline_index + i].order_line.OL_O_ID	= orders_region[new_order_index].orders.O_ID;
			order_line_region[new_orderline_index + i].order_line.OL_I_ID	= requested_item_id;
		}
		DEBUG_COUT("Added record(s) to table ORDERLINE");
		
		// Insert ccxacts
		new_ccxacts_index = commit_timestamp.timestamp - 1;
		memcpy(&cc_xacts_region[new_ccxacts_index].cc_xacts, &(ctx.commit_request->ccxact), sizeof(CCXacts));
		cc_xacts_region[new_ccxacts_index].write_timestamp	= commit_timestamp.timestamp;
		cc_xacts_region[new_ccxacts_index].cc_xacts.CX_O_ID	= orders_region[new_order_index].orders.O_ID;
		DEBUG_COUT("Added record to table CC_XACTS");
		
		// Release the lock
		item_lock[requested_item_id].unlock();
		DEBUG_COUT("Released the lock on item " << requested_item_id);
		
		
		// ************************************************************************
		//	Return the result of the commit to user
		ctx.commit_response->commit_outcome = CommitResponse::COMMITTED;
		
		
		// if server expects more requests from the user, it must submit RECEIVE job
		if (ctx.transaction_statement_number  <  TRANSACTION_CNT)
			TEST_NZ (RDMACommon::post_RECEIVE(ctx.qp, ctx.item_info_req_mr, (uintptr_t)ctx.item_info_request, sizeof(struct ItemInfoRequest)));
			
		TEST_NZ (RDMACommon::post_SEND (ctx.qp, ctx.commit_res_mr, (uintptr_t)ctx.commit_response, sizeof(struct CommitResponse)));
		TEST_NZ (RDMACommon::poll_completion(ctx.cq));		// completion for post_SEND
		DEBUG_COUT("Result of the commit successfully sent to user"); 
	}
	
	/* Sync so server will know that client is done mucking with its memory */
	TEST_NZ (sock_sync_data (ctx.client_sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	std::cout << "final value of the timestamp buffer " << Server::timestamp_region.timestamp << std::endl;
	//std::cout << "final value of lock  " << Lock::get_lock_status(lock_items_region[0]) << " | " << Lock::get_version(lock_items_region[0]) << std::endl;
	
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
	
	close(Server::server_sockfd);	// close the socket
	return 0;
}

int Server::initialize_data_structures(){
	
	Server::timestamp_region.timestamp = 0ULL;	// the timestamp counter is initially set to 0
	DEBUG_COUT("Timestamp set to " << Server::timestamp_region.timestamp);

	TEST_NZ(load_tables_from_files());
	DEBUG_COUT("tables loaded successfully");
	
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
			std::cerr << "Cannot open file: " << ITEM_FILENAME << std::endl;
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
	    std::std::cerr << "exception caught: " << e.what() << '\n';
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
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " starts a server and wait for connection" << std::endl;
}

int Server::start_server ()
{	
	TEST_NZ(initialize_data_structures());
	
	std::cout << "waiting for " << CLIENTS_CNT << " client(s) on port " << TCP_PORT << " for TCP connection!" << std::endl;
	
	Server::server_sockfd = -1;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	pthread_t master_threads[CLIENTS_CNT];	
	int client_socks[CLIENTS_CNT];
	
	// Open Socket
	Server::server_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (Server::server_sockfd < 0)
	{
		std::cerr << "Error opening socket" << std::endl;
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
			std::cerr << "ERROR on accept" << std::endl;
			return -1;
		}
		std::cout << "received client #" << i << std::endl;
		pthread_create(&master_threads[i], NULL, Server::handle_client, &client_socks[i]);
		i++;
	}
	
	//wait for handlers to finish
	for (i = 0; i < CLIENTS_CNT; i++) {
		pthread_join(master_threads[i], NULL);
	}
	
	// close server socket
	std::cout << "Server is done, now destroying resources!" << std::endl;
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