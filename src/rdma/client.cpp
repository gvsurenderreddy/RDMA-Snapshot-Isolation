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
	int items_size;
	int orders_size;
	int cc_xacts_size;
	int ts_size;
	int lock_size;
	
	ctx->recv_msg = new Message[1];
	
	ctx->local_items_region				= new ItemVersion[FETCH_BLOCK_SIZE];
	ctx->local_orders_region			= new OrdersVersion[1];
	ctx->local_cc_xacts_region			= new CCXactsVersion[1];
	ctx->local_read_timestamp_region	= new TimestampOracle[1];	
	ctx->local_commit_timestamp_region	= new TimestampOracle[1];	
	ctx->local_lock_items_region		= new uint64_t[1];	
	
	items_size		= FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	orders_size		= sizeof(OrdersVersion);
	cc_xacts_size	= sizeof(CCXactsVersion);
	ts_size			= sizeof(TimestampOracle);
	lock_size		= sizeof(uint64_t);
	
	mr_flags = IBV_ACCESS_LOCAL_WRITE;
	// mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
	
	
	TEST_Z(ctx->recv_mr						= ibv_reg_mr(ctx->pd, ctx->recv_msg, sizeof(struct Message), mr_flags));
	TEST_Z(ctx->local_items_mr				= ibv_reg_mr(ctx->pd, ctx->local_items_region, items_size, mr_flags));
	TEST_Z(ctx->local_orders_mr				= ibv_reg_mr(ctx->pd, ctx->local_orders_region, orders_size, mr_flags));
	TEST_Z(ctx->local_cc_xacts_mr			= ibv_reg_mr(ctx->pd, ctx->local_cc_xacts_region, cc_xacts_size, mr_flags));
	TEST_Z(ctx->local_read_timestamp_mr		= ibv_reg_mr(ctx->pd, ctx->local_read_timestamp_region, ts_size, mr_flags));
	TEST_Z(ctx->local_commit_timestamp_mr	= ibv_reg_mr(ctx->pd, ctx->local_commit_timestamp_region, ts_size, mr_flags));
	TEST_Z(ctx->local_lock_items_mr			= ibv_reg_mr(ctx->pd, ctx->local_lock_items_region, lock_size, mr_flags));
	
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
	
	if (ctx->recv_mr)
		TEST_NZ (ibv_dereg_mr (ctx->recv_mr));
	
	if (ctx->local_items_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_items_mr));
			
	if (ctx->local_orders_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_orders_mr));
	
	if (ctx->local_cc_xacts_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_cc_xacts_mr));
	
	if (ctx->local_read_timestamp_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_read_timestamp_mr));
	
	if (ctx->local_commit_timestamp_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_commit_timestamp_mr));
	
	if (ctx->local_lock_items_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_lock_items_mr));
	
	
	delete[](ctx->recv_msg);
	delete[](ctx->local_items_region);
	delete[](ctx->local_orders_region);
	delete[](ctx->local_cc_xacts_region);
	delete[](ctx->local_read_timestamp_region);
	delete[](ctx->local_commit_timestamp_region);
	delete[](ctx->local_lock_items_region);
	
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

int Client::check_if_version_is_in_block(int version, ItemVersion *items_region, int size)
{
	int biggest_eligible_index = -1;
	int i;
	for (i=0; i < size; i++){
		if (items_region[i].write_timestamp <= version){
			// TODO: Ensure the validity of this section
			biggest_eligible_index = i;
			break;
		}
	}
	// TODO WRONGGGG
	biggest_eligible_index = 0;
	
	return biggest_eligible_index;
}


int Client::start_transaction(Context *ctx)
{
	int item_id;
	uint32_t lock_status, version;
	uint64_t expected_lock, new_lock;
	int abort_cnt = 0;
	
	ctx->transaction_statement_number = 0;
	DEBUG_COUT ("Transaction now gets started");
	
	while (ctx->transaction_statement_number  <  TRANSACTION_STATEMENT_CNT){
		// ************************************************
		//	Acquiring read timestamp
		// ************************************************
		ctx->transaction_statement_number = ctx->transaction_statement_number + 1;
		DEBUG_COUT (endl << "Handling transaction #" << ctx->transaction_statement_number);
	
		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
		ctx->qp,
		ctx->local_read_timestamp_mr,
		(uint64_t)ctx->local_read_timestamp_region,
		&(ctx->peer_mr_timestamp),
		(uint64_t)ctx->peer_mr_timestamp.addr,
		(uint32_t)sizeof(uint64_t),
		true));
	
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));
	
		DEBUG_COUT ("Step 1: Received read timestamp from oracle with TID " << ctx->local_read_timestamp_region[0].timestamp);
	
		// ************************************************
		//	Fetching ITEM information
		// ************************************************
		item_id = rand() % MAX_ITEM_CNT;	// generating in the range 0 to MAX_ITEM_CNT
	
		// Find the lookup address on the server remote memory 
		int item_offset = (item_id * MAX_ITEM_VERSIONS * sizeof(ItemVersion))
			+ ctx->fetch_block_num * FETCH_BLOCK_SIZE * sizeof(ItemVersion);
		ItemVersion *item_lookup_address =  (ItemVersion *)(item_offset + ((uint64_t)ctx->peer_mr_items.addr));
	
		// RDMA READ it from the server
		TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
		ctx->qp,
		ctx->local_items_mr,
		(uint64_t)ctx->local_items_region,
		&(ctx->peer_mr_items),
		(uint64_t)item_lookup_address,
		FETCH_BLOCK_SIZE * sizeof(ItemVersion),
		true));
	
		TEST_NZ (RDMACommon::poll_completion(ctx->cq));
		DEBUG_COUT ("Step 2: Received information for item " << item_id <<  ". checking if the suitable version is among them!");
	
		ItemVersion *found_version = new ItemVersion[1];
		int found_version_index = Client::check_if_version_is_in_block(ctx->local_read_timestamp_region[0].timestamp, ctx->local_items_region, MAX_ITEM_VERSIONS);
	
		if (found_version_index >= 0)
		{
			// Found the version in the received block
			DEBUG_COUT("....... Version Found in block " << ctx->fetch_block_num << " with timestamp " << ctx->local_items_region[found_version_index].write_timestamp);
		
			// ************************************************
			//	Commit begins
			// ************************************************
		
		
			// ************************************************
			//	Acquiring commit timestamp
			ctx->local_commit_timestamp_region[0].timestamp = 4ULL;
			TEST_NZ (RDMACommon::send_RDMA_FETCH_ADD(ctx->qp,
			ctx->local_commit_timestamp_mr,
			(uint64_t)ctx->local_commit_timestamp_region,
			&(ctx->peer_mr_timestamp),
			(uint64_t)ctx->peer_mr_timestamp.addr,
			(uint64_t)1ULL,
			(uint32_t)sizeof(TimestampOracle)));
	
			TEST_NZ (RDMACommon::poll_completion(ctx->cq));
			
			// Byte swapping, since all values read by atomic operations are in reverse bit order
			ctx->local_commit_timestamp_region[0].timestamp = hton64(ctx->local_commit_timestamp_region[0].timestamp);
			
			ctx->local_commit_timestamp_region[0].timestamp += 1ULL;
			DEBUG_COUT ("Step 3: Commit timestamp " << ctx->local_commit_timestamp_region[0].timestamp << " successfully acquired.");	
		

			// ************************************************
			// First step, acquiring locks
	
			// lock expected before locking 
			lock_status		= (uint32_t) 0;
			version			= (uint32_t) ctx->local_items_region[found_version_index].write_timestamp;
			expected_lock	= Lock::set_lock(lock_status, version);
			
			// new lock
			lock_status		= (uint32_t) 1;
			version			= (uint32_t) ctx->local_commit_timestamp_region[0].timestamp;			
			new_lock		= Lock::set_lock(lock_status, version);
			
			// and the actual content of the lock (before being swapped) will be stored in the following variable
			memset(ctx->local_lock_items_region, 0, sizeof(uint64_t));
			
			int lock_offset = (item_id) * sizeof(uint64_t);
			uint64_t *lock_lookup_address =  (uint64_t *)(lock_offset + ((uint64_t)ctx->peer_mr_lock_items.addr));
		
			TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(ctx->qp,
			ctx->local_lock_items_mr,
			(uint64_t)ctx->local_lock_items_region,
			&(ctx->peer_mr_lock_items),
			(uint64_t)lock_lookup_address,
			(uint32_t)sizeof(uint64_t),
			(uint64_t)expected_lock,
			(uint64_t)new_lock));
			
			TEST_NZ (RDMACommon::poll_completion(ctx->cq));
			
			// Byte swapping, since all values read by atomic operations are in reverse bit order
			ctx->local_lock_items_region[0] = hton64(ctx->local_lock_items_region[0]);
			
			//DEBUG_COUT ("Expected lock: " << Lock::get_lock_status(expected_lock) << " | " << Lock::get_version(expected_lock));
			//DEBUG_COUT ("Existing lock: " << Lock::get_lock_status(ctx->local_lock_items_region[0]) << " | " << Lock::get_version(ctx->local_lock_items_region[0]));
			//DEBUG_COUT ("New lock:		" << Lock::get_lock_status(new_lock) << " | " << Lock::get_version(new_lock));
			
			if (Lock::are_equals(ctx->local_lock_items_region[0], expected_lock))
			{
				DEBUG_COUT ("Step 4: Lock on item " << item_id << " successfully acquired.");
				
				// ************************************************
				//	Add a new record to table ORDERS
				ctx->local_orders_region->write_timestamp	= ctx->local_commit_timestamp_region[0].timestamp;
				ctx->local_orders_region->orders.O_ID		= ctx->local_commit_timestamp_region[0].timestamp;
				ctx->local_orders_region->orders.O_I_ID	= item_id;
	
				int order_offset = ((ctx->local_orders_region->orders.O_ID - 1) * sizeof(OrdersVersion));
				OrdersVersion *orderes_lookup_address =  (OrdersVersion *)(order_offset + (uint64_t)ctx->peer_mr_orders.addr);
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx->qp,
				ctx->local_orders_mr,
				(uint64_t)ctx->local_orders_region,
				&(ctx->peer_mr_orders),
				(uint64_t)orderes_lookup_address,
				(uint32_t)sizeof(OrdersVersion),
				false));
				DEBUG_COUT ("Step 5: Successfully added a new record to table ORDERS (unsignaled)");
			
			
				// ************************************************
				//	Adding a new record to CC_XACTS table
				ctx->local_cc_xacts_region->write_timestamp = ctx->local_commit_timestamp_region[0].timestamp;
				ctx->local_cc_xacts_region->cc_xacts.CX_O_ID = ctx->local_orders_region->orders.O_ID;
		
				int cc_offset = ((ctx->local_cc_xacts_region->cc_xacts.CX_O_ID - 1) * sizeof(CCXactsVersion));
				CCXactsVersion *cc_lookup_address =  (CCXactsVersion *)(cc_offset + (uint64_t)ctx->peer_mr_cc_xacts.addr);
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx->qp,
				ctx->local_cc_xacts_mr,
				(uint64_t)ctx->local_cc_xacts_region,
				&(ctx->peer_mr_cc_xacts),
				(uint64_t)cc_lookup_address,
				(uint32_t)sizeof(CCXactsVersion),
				false));
				DEBUG_COUT ("Step 6: Successfully added a new record to table CC_XACTS (unsignaled)");
			
			
				// ************************************************
				//	Decrementing the stock in ITEM table
				ctx->local_items_region[0].write_timestamp = ctx->local_commit_timestamp_region[0].timestamp;
				if (ctx->local_items_region[0].item.I_STOCK < 10)
					ctx->local_items_region[0].item.I_STOCK += 20;
				else
					ctx->local_items_region[0].item.I_STOCK -= 1;
	
				// we already have item_lookup_address
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx->qp,
				ctx->local_items_mr,
				(uint64_t)ctx->local_items_region,
				&(ctx->peer_mr_items),
				(uint64_t)item_lookup_address,
				(uint32_t)sizeof(ItemVersion),
				false));
				DEBUG_COUT ("Step 7: Successfully decremented the stock in table ITEM (unsignaled)");


				// ************************************************
				//	Releasing the lock
				
				lock_status		= (uint32_t) 0;
				version			= (uint32_t) ctx->local_commit_timestamp_region[0].timestamp;
				ctx->local_lock_items_region[0] = Lock::set_lock(lock_status, version);	
				
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx->qp,
				ctx->local_lock_items_mr,
				(uint64_t)ctx->local_lock_items_region,
				&(ctx->peer_mr_lock_items),
				(uint64_t)lock_lookup_address,
				(uint32_t)sizeof(uint64_t),
				true));
				TEST_NZ (RDMACommon::poll_completion(ctx->cq));
				DEBUG_COUT ("Step 8: Lock released.");
				DEBUG_COUT ("Transaction finished successfully!");
			}
			else
			{
				DEBUG_CERR ("ABORT: requested item is either locked or changed by some other transaction. Lock on item could not get acquired :(");
				DEBUG_CERR ("Details:");
				DEBUG_CERR ("...Lock: " << Lock::get_lock_status(ctx->local_lock_items_region[0]) << " | " << Lock::get_version(ctx->local_lock_items_region[0]));		
				DEBUG_COUT ("...but was looking for version " << ctx->local_items_region[found_version_index].write_timestamp);
				abort_cnt++;
				continue;
			}
		}
		else{
			// TODO				
			DEBUG_COUT ("Version NOT Found: in block " << ctx-> fetch_block_num);
			return -1;
		}
	}
	
	int committed_cnt = TRANSACTION_STATEMENT_CNT - abort_cnt;
	double success_rate = (double)committed_cnt /  TRANSACTION_STATEMENT_CNT;
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
	
	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(ctx.qp, ctx.recv_mr, (uintptr_t)ctx.recv_msg, sizeof(struct Message));
	
	TEST_NZ(connect_qp (&ctx));
	
	TEST_NZ(RDMACommon::poll_completion(ctx.cq));
	// after receiving the message from the server, let's store its addresses in the context
	memcpy(&ctx.peer_mr_items, &ctx.recv_msg->mr_items, sizeof(ctx.peer_mr_items));
	memcpy(&ctx.peer_mr_orders, &ctx.recv_msg->mr_orders, sizeof(ctx.peer_mr_orders));
	memcpy(&ctx.peer_mr_cc_xacts, &ctx.recv_msg->mr_cc_xacts, sizeof(ctx.peer_mr_cc_xacts));
	memcpy(&ctx.peer_mr_timestamp, &ctx.recv_msg->mr_timestamp_oracle, sizeof(ctx.peer_mr_timestamp));
	memcpy(&ctx.peer_mr_lock_items, &ctx.recv_msg->mr_lock_items, sizeof(ctx.peer_mr_lock_items));
	
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