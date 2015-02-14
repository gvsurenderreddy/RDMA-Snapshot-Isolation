/*
 *	rdma-client.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "rdma-client.hpp"
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


// Definitions
RDMAClient::ShoppingCart	RDMAClient::shopping_cart;
int							RDMAClient::transaction_statement_number = 0;
RDMAClient::SharedContext	RDMAClient::shared_context;	

int RDMAClient::create_context (struct Context *ctx)
{
	TEST_NZ(build_connection(ctx));
	TEST_NZ(register_memory(ctx));
	TEST_NZ(RDMACommon::create_queuepair(ctx->ib_ctx, ctx->pd, ctx->cq, &(ctx->qp)));
	
	return 0;
}

int RDMAClient::build_connection(Context *ctx)
{
	struct ibv_device **dev_list = NULL;
	struct ibv_device *ib_dev = NULL;
	int cq_size = 0;
	int num_devices;
	
	ctx->client_sockfd = sock_connect (ctx->server_address, ctx->tcp_port);
	if (ctx->client_sockfd < 0)
	{
		std::cerr << "failed to establish TCP connection to server " <<  ctx->server_address << ", port " << ctx->tcp_port << std::endl;
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
	
	TEST_NZ (ibv_query_port (ctx->ib_ctx, ctx->ib_port, &ctx->port_attr));	// query port properties
	
	TEST_Z(ctx->pd = ibv_alloc_pd (ctx->ib_ctx));		// allocate Protection Domain
	
	// Create completion channel and completion queue
	TEST_Z(ctx->comp_channel = ibv_create_comp_channel(ctx->ib_ctx));
	cq_size = 1000;	// the size of the completion queue
	TEST_Z(ctx->cq = ibv_create_cq (ctx->ib_ctx, cq_size, NULL, ctx->comp_channel, 0));
	
	return 0;
}


int RDMAClient::register_memory(Context *ctx)
{
	int mr_flags = 0;
	int i_s;
	int o_s;
	int ol_s;
	int cc_s;
	int ts_s;
	int lock_s;
	
	ctx->recv_msg = new Message[1];
	
	ctx->local_items_region				= new ItemVersion[FETCH_BLOCK_SIZE];
	ctx->local_order_line_region		= new OrderLineVersion[ORDERLINE_PER_ORDER];
	ctx->local_lock_items_region		= new uint64_t[1];	
	
	i_s		= FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	o_s		= sizeof(OrdersVersion);
	ol_s	= ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);
	cc_s	= sizeof(CCXactsVersion);
	ts_s	= sizeof(TimestampOracle);
	lock_s	= sizeof(uint64_t);
	
	mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	TEST_Z(ctx->recv_mr						= ibv_reg_mr(ctx->pd, ctx->recv_msg, sizeof(struct Message), mr_flags));
	TEST_Z(ctx->local_items_mr				= ibv_reg_mr(ctx->pd, ctx->local_items_region, i_s, mr_flags));
	TEST_Z(ctx->local_orders_mr				= ibv_reg_mr(ctx->pd, &ctx->local_orders_region, o_s, mr_flags));
	TEST_Z(ctx->local_order_line_mr			= ibv_reg_mr(ctx->pd, ctx->local_order_line_region, ol_s, mr_flags));
	TEST_Z(ctx->local_cc_xacts_mr			= ibv_reg_mr(ctx->pd, &ctx->local_cc_xacts_region, cc_s, mr_flags));
	TEST_Z(ctx->local_read_timestamp_mr		= ibv_reg_mr(ctx->pd, &ctx->local_read_timestamp_region, ts_s, mr_flags));
	TEST_Z(ctx->local_commit_timestamp_mr	= ibv_reg_mr(ctx->pd, &ctx->local_commit_timestamp_region, ts_s, mr_flags));
	TEST_Z(ctx->local_lock_items_mr			= ibv_reg_mr(ctx->pd, ctx->local_lock_items_region, lock_s, mr_flags));
	
	return 0;
}

int RDMAClient::connect_qp (struct Context *ctx)
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
	TEST_NZ(RDMACommon::modify_qp_to_init (ctx->ib_port, ctx->qp));
	
	// modify the QP to RTR
	TEST_NZ(RDMACommon::modify_qp_to_rtr (ctx->ib_port, ctx->qp, remote_con_data.qp_num, remote_con_data.lid, remote_con_data.gid));
	
	// modify the QP to RTS
	TEST_NZ(RDMACommon::modify_qp_to_rts (ctx->qp));
	
	// sync to make sure that both sides are in states that they can connect to prevent packet loss
	TEST_NZ(sock_sync_data (ctx->client_sockfd, 1, "Q", &temp_char));	// just send a dummy char back and forth
	
	return 0;
}

int RDMAClient::destroy_context (struct Context *ctx)
{
	if (ctx->qp)
		TEST_NZ(ibv_destroy_qp (ctx->qp));
	
	if (ctx->recv_mr)
		TEST_NZ (ibv_dereg_mr (ctx->recv_mr));
	
	if (ctx->local_items_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_items_mr));
			
	if (ctx->local_orders_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_orders_mr));
	
	if (ctx->local_order_line_mr)
		TEST_NZ (ibv_dereg_mr (ctx->local_order_line_mr));
	
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
	delete[](ctx->local_order_line_region);
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

void RDMAClient::die(const char *reason)
{
	std::cerr <<  reason << std::endl;
	std::cerr << "Errno: " << strerror(errno) << std::endl;	
	exit(EXIT_FAILURE);
}

void RDMAClient::usage (const char *argv0)
{
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

int RDMAClient::check_if_version_is_in_block(int version, ItemVersion *items_region, int size)
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

void RDMAClient::fill_shopping_cart(Context *ctx)
{
	int item_id;
	int quantity;
	
	DEBUG_COUT ("Cart contents: (Item ID,  Quantity)");
	for (int i=0; i < ORDERLINE_PER_ORDER; i++)
	{
		item_id		= (i * ITEM_PER_SERVER) + (rand() % ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		quantity	= (rand() % 1) + 1;			// the quanity of the item (not importatn)
		RDMAClient::shopping_cart.cart_lines[i].SCL_I_ID	= item_id;
		RDMAClient::shopping_cart.cart_lines[i].SCL_QTY		= quantity;
		DEBUG_COUT (".... " <<  item_id << '\t' << quantity);
	
		ctx[i].associated_cart_line = &RDMAClient::shopping_cart.cart_lines[i];
	}
}

void* RDMAClient::get_item_info(Context *ctx)
{
	ctx->fetch_block_num = 0;
	int item_id = ctx->associated_cart_line->SCL_I_ID;
	
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
}

void* RDMAClient::acquire_item_lock(Context *ctx, uint64_t *expected_lock, uint64_t *new_lock)
{
	int item_id = ctx->associated_cart_line->SCL_I_ID;
	
	// and the actual content of the lock (before being swapped) will be stored in the following variable
	memset(ctx->local_lock_items_region, 0, sizeof(uint64_t));
	
	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx->peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(ctx->qp,
	ctx->local_lock_items_mr,
	(uint64_t)ctx->local_lock_items_region,
	&(ctx->peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	(uint64_t)expected_lock,
	(uint64_t)new_lock));
}

void* RDMAClient::revert_lock(Context *ctx, uint64_t *new_lock)
{
	int item_id		= ctx->associated_cart_line->SCL_I_ID;
	
	memcpy(ctx->local_lock_items_region, new_lock, sizeof(uint64_t));	
		
	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx->peer_mr_lock_items.addr));
		
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx->qp,
	ctx->local_lock_items_mr,
	(uint64_t)ctx->local_lock_items_region,
	&(ctx->peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	true));
}


void* RDMAClient::decrement_item_stock(Context *ctx)
{
	int item_id		= ctx->associated_cart_line->SCL_I_ID;
	int quantity	= ctx->associated_cart_line->SCL_QTY;
	
	ctx->local_items_region[0].write_timestamp = shared_context.commit_timestamp;
	ctx->local_items_region[0].item.I_STOCK -= quantity;
	
	if (ctx->local_items_region[0].item.I_STOCK < 10)
		ctx->local_items_region[0].item.I_STOCK += 20;
	
	// Find the lookup address on the server remote memory 
	int item_offset = (item_id * MAX_ITEM_VERSIONS * sizeof(ItemVersion))
		+ ctx->fetch_block_num * FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	
	ItemVersion *item_lookup_address =  (ItemVersion *)(item_offset + ((uint64_t)ctx->peer_mr_items.addr));
	
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx->qp,
	ctx->local_items_mr,
	(uint64_t)ctx->local_items_region,
	&(ctx->peer_mr_items),
	(uint64_t)item_lookup_address,
	(uint32_t)sizeof(ItemVersion),
	false));

	DEBUG_COUT (".... Successfully decremented the stock for item " << item_id
		<< " (with commit timestamp " << shared_context.commit_timestamp << ")");
}

void* RDMAClient::release_lock(Context *ctx)
{
	int item_id		= ctx->associated_cart_line->SCL_I_ID;
	uint32_t		lock_status, version;
	
	lock_status		= (uint32_t) 0;
	version			= (uint32_t) shared_context.commit_timestamp;
	ctx->local_lock_items_region[0] = Lock::set_lock(lock_status, version);	

	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx->peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx->qp,
	ctx->local_lock_items_mr,
	(uint64_t)ctx->local_lock_items_region,
	&(ctx->peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	true));
}

int RDMAClient::start_transaction(Context *ctx)
{
	int		server_num;
	int		abort_cnt = 0;
	bool	abort_flag;
    bool	successful_locking_servers[SERVER_CNT] = { false };		// initializes the entire array to false 
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	uint32_t	lock_status, version;
	uint64_t	expected_lock[SERVER_CNT], new_lock[SERVER_CNT];
	
	
	DEBUG_COUT ("Transaction now gets started");
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	
	while (transaction_statement_number  <  TRANSACTION_CNT){
		// ************************************************
		//	Acquiring read timestamp
		// ************************************************
		transaction_statement_number = transaction_statement_number + 1;
		DEBUG_COUT (std::endl << "Handling transaction #" << transaction_statement_number);
		
		fill_shopping_cart(ctx);
	
		// the first server is responsible for timestamps (timestamp oracle)
		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
		ctx[0].qp,
		ctx[0].local_read_timestamp_mr,
		(uint64_t)&ctx[0].local_read_timestamp_region,
		&(ctx[0].peer_mr_timestamp),
		(uint64_t)ctx[0].peer_mr_timestamp.addr,
		(uint32_t)sizeof(uint64_t),
		true));
		TEST_NZ (RDMACommon::poll_completion(ctx[0].cq));
		
		shared_context.read_timestamp = ctx[0].local_read_timestamp_region.timestamp;
		
		// copy to all contexts
	
		DEBUG_COUT ("Step 1: Received read timestamp from oracle with TID " << shared_context.read_timestamp);
	
	
		// ************************************************
		//	Fetching ITEMs information
		// ************************************************
		
		// TODO: Should be fixed. multiple requests to the same server should be sent with only one signalled request 
		for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
		{
			// first find which server the data corresponds to
			server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
			get_item_info(&ctx[server_num]);
		}
		
		for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
		{
			server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
			TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
			DEBUG_COUT (".... Received information for item " << ctx[server_num].local_items_region->item.I_ID << " from " << ctx[server_num].server_address);
		}
		DEBUG_COUT ("Step 2: Received information for all items");
		
		
		// Since we only have one version per item (and hence one version is read for each item), we simplify this section
		/*
		int found_version_index = RDMAClient::check_if_version_is_in_block(ctx->local_read_timestamp_region[0].timestamp, ctx->local_items_region, MAX_ITEM_VERSIONS);
		if (found_version_index >= 0)
		*/
		int found_version_index = 0;
		if (true)	
		{
			// Found the version in the received block
			// DEBUG_COUT("....... Version Found in block " << ctx[0].fetch_block_num << " with timestamp " << ctx[0].local_items_region[found_version_index].write_timestamp);
		
			// ************************************************
			//	Commit begins
			// ************************************************
		
		
			// ************************************************
			//	Acquiring commit timestamp
			
			TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(ctx[0].qp,
			ctx[0].local_commit_timestamp_mr,
			(uint64_t)&ctx[0].local_commit_timestamp_region,
			&(ctx[0].peer_mr_timestamp),
			(uint64_t)ctx[0].peer_mr_timestamp.addr,
			(uint64_t)1ULL,
			(uint32_t)sizeof(TimestampOracle)));
			TEST_NZ (RDMACommon::poll_completion(ctx[0].cq));
			
			// Byte swapping, since all values read by atomic operations are in reverse bit order
			ctx[0].local_commit_timestamp_region.timestamp = hton64(ctx[0].local_commit_timestamp_region.timestamp);
			ctx[0].local_commit_timestamp_region.timestamp += 1ULL;
			shared_context.commit_timestamp = ctx[0].local_commit_timestamp_region.timestamp;
			DEBUG_COUT ("Step 3: Commit timestamp " << shared_context.commit_timestamp << " successfully acquired.");
		
			
			// ************************************************
			// First step, acquiring locks
			for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
			{
				server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
				
				// lock expected before locking 
				lock_status					= (uint32_t) 0;
				version						= (uint32_t) ctx[server_num].local_items_region[found_version_index].write_timestamp;
				expected_lock[server_num]	= Lock::set_lock(lock_status, version);
	
				// new lock
				lock_status					= (uint32_t) 1;
				version						= (uint32_t) shared_context.commit_timestamp;			
				new_lock[server_num]		= Lock::set_lock(lock_status, version);
				
				acquire_item_lock(&ctx[server_num], &expected_lock[server_num], &new_lock[server_num]);
			}
			
			abort_flag = false;
			for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
			{
				server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
				
				TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
	
				// Byte swapping, since all values read by atomic operations are in reverse bit order
				ctx[server_num].local_lock_items_region[0]	= hton64(ctx[server_num].local_lock_items_region[0]);
	
				// Check if Compare-and-swap was successful
				if (Lock::are_equals(ctx[server_num].local_lock_items_region[0], expected_lock[server_num]))
				{
					DEBUG_COUT (".... Lock on item " << shopping_cart.cart_lines[i].SCL_I_ID << " successfully acquired.");
					successful_locking_servers[server_num] = true;
				}
				else
				{
					DEBUG_COUT (".... Failed to lock item " << shopping_cart.cart_lines[i].SCL_I_ID);
					DEBUG_COUT ("........ Expected lock: " << Lock::get_lock_status(expected_lock[server_num])
						<< " | " << Lock::get_version(expected_lock[server_num]));
					DEBUG_COUT ("........ Existing lock: " << Lock::get_lock_status(ctx[server_num].local_lock_items_region[0])
						<< " | " << Lock::get_version(ctx[server_num].local_lock_items_region[0]));
				
					successful_locking_servers[server_num] = false;					
					abort_flag = true;
				}	
			}
			
			// Check if all item locks are successfully acquired 
			if (abort_flag == true)
			{
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true)
						revert_lock(&ctx[server_num], &expected_lock[server_num]);
				}
				
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true)
					{
						TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
						DEBUG_COUT (".... Lock reverted on item " << ctx[server_num].local_items_region->item.I_ID);
					}
				}
				DEBUG_CERR ("ABORT: Lock on all items could not be acquired :(");
				abort_cnt++;
				continue;
			}
			else
			{
				DEBUG_COUT ("Step 4: All locks successfully acquired");
				
				// ************************************************
				//	Decrementing the stock in ITEM table
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					decrement_item_stock(&ctx[server_num]);
				}				
				DEBUG_COUT ("Step 5: Successfully decremented the stock in table ITEM (unsignaled)");
				
				
				// ************************************************
				//	Releasing the lock
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					release_lock(&ctx[server_num]);
				}
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					server_num = shopping_cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					
					TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
					DEBUG_COUT (".... Lock on item " << shopping_cart.cart_lines[i].SCL_I_ID  << " released with commit timestamp " << shared_context.commit_timestamp);
				}					
				DEBUG_COUT ("Step 6: All locks successfully released");
				
				
				// ************************************************
				//	Add a new record to table ORDERS (which is stored on only one server)
				ctx[0].local_orders_region.write_timestamp	= shared_context.commit_timestamp;
				ctx[0].local_orders_region.orders.O_ID		= shared_context.commit_timestamp;
				
				int order_offset = ((ctx[0].local_orders_region.orders.O_ID - 1) * sizeof(OrdersVersion));
				OrdersVersion *orders_lookup_address =  (OrdersVersion *)(order_offset + (uint64_t)ctx[0].peer_mr_orders.addr);
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].local_orders_mr,
				(uint64_t)&ctx[0].local_orders_region,
				&(ctx[0].peer_mr_orders),
				(uint64_t)orders_lookup_address,
				(uint32_t)sizeof(OrdersVersion),
				false));
				DEBUG_COUT ("Step 7: Successfully added a new record to table ORDERS (unsignaled)");
				
				
				// ************************************************
				//	Add new record(s) to table ORDERLINE (which is stored on only one server)
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++)
				{
					ctx[0].local_order_line_region[i].write_timestamp		= shared_context.commit_timestamp;
					ctx[0].local_order_line_region[i].order_line.OL_ID		= (shared_context.commit_timestamp - 1) * ORDERLINE_PER_ORDER + 1 + i;
					ctx[0].local_order_line_region[i].order_line.OL_O_ID	= ctx[0].local_orders_region.orders.O_ID;
					ctx[0].local_order_line_region[i].order_line.OL_I_ID	= shopping_cart.cart_lines[i].SCL_I_ID;
					ctx[0].local_order_line_region[i].order_line.OL_QTY		= shopping_cart.cart_lines[i].SCL_QTY;
				}
				int ol_offset = (shared_context.commit_timestamp- 1) * ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);
				OrderLineVersion *ol_lookup_address = (OrderLineVersion *)(ol_offset + (uint64_t)ctx[0].peer_mr_order_line.addr);
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].local_order_line_mr,
				(uint64_t)ctx[0].local_order_line_region,
				&(ctx[0].peer_mr_order_line),
				(uint64_t)ol_lookup_address,
				(uint32_t)ORDERLINE_PER_ORDER * sizeof(OrderLineVersion),
				false));
				DEBUG_COUT ("Step 8: Successfully added new record(s) to table ORDERLINE (unsignaled)");
		
				
				// ************************************************
				//	Adding a new record to CC_XACTS table (which is stored on only one server)
				ctx[0].local_cc_xacts_region.write_timestamp = shared_context.commit_timestamp;
				ctx[0].local_cc_xacts_region.cc_xacts.CX_O_ID = ctx[0].local_orders_region.orders.O_ID;
		
				int cc_offset = ((shared_context.commit_timestamp - 1) * sizeof(CCXactsVersion));
				CCXactsVersion *cc_lookup_address =  (CCXactsVersion *)(cc_offset + (uint64_t)ctx[0].peer_mr_cc_xacts.addr);
			
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].local_cc_xacts_mr,
				(uint64_t)&ctx[0].local_cc_xacts_region,
				&(ctx[0].peer_mr_cc_xacts),
				(uint64_t)cc_lookup_address,
				(uint32_t)sizeof(CCXactsVersion),
				true));
				TEST_NZ (RDMACommon::poll_completion(ctx->cq));
				DEBUG_COUT ("Step 9: Successfully added a new record to table CC_XACTS (signaled)");
			
				DEBUG_COUT ("Transaction finished successfully!");
			}
		}
	}
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	
	double nano_elapsed_time = ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec );
	double T_P_MILISEC = (double)(TRANSACTION_CNT / (double)(nano_elapsed_time / 1000000));
	std::cout << "Transaction per millisec: " <<  T_P_MILISEC << std::endl;
	
	int committed_cnt = TRANSACTION_CNT - abort_cnt;
	double success_rate = (double)committed_cnt /  TRANSACTION_CNT;
	std::cout << committed_cnt << " committed, " << abort_cnt << " aborted (success rate = " << success_rate << ")." << std::endl;
	
	return 0;
}


int RDMAClient::start_client ()
{	
	// Init Context
	struct Context ctx[SERVER_CNT];
	char temp_char;
	
	for (int i = 0; i < SERVER_CNT; i++){
	    memset (&ctx[i], 0, sizeof(Context));
	    ctx[i].client_sockfd = -1;
	   	strcpy(ctx[i].server_address, SERVER_ADDR[i].c_str());
		ctx[i].tcp_port	= TCP_PORT[i];
		ctx[i].ib_port	= IB_PORT[i];
		
	
		TEST_NZ (create_context (&ctx[i]));
	
		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(ctx[i].qp, ctx[i].recv_mr, (uintptr_t)ctx[i].recv_msg, sizeof(struct Message));
	
		TEST_NZ(connect_qp (&ctx[i]));
	
		TEST_NZ(RDMACommon::poll_completion(ctx[i].cq));
		// after receiving the message from the server, let's store its addresses in the context
		memcpy(&ctx[i].peer_mr_items,		&ctx[i].recv_msg->mr_items,			sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_orders,		&ctx[i].recv_msg->mr_orders,		sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_order_line,	&ctx[i].recv_msg->mr_order_line,	sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_cc_xacts,	&ctx[i].recv_msg->mr_cc_xacts,		sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_timestamp,	&ctx[i].recv_msg->mr_timestamp,		sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_lock_items,	&ctx[i].recv_msg->mr_lock_items,	sizeof(struct ibv_mr));
	}
	
	std::cout << "Successfully connected to all servers." << std::endl;
	
    srand (time(NULL));		// initialize random seed
	
	start_transaction(ctx);
	
	/* Sync so server will know that client is done mucking with its memory */
	DEBUG_COUT("RDMAClient is done, and is ready to destroy its resources!");
	for (int i = 0; i < SERVER_CNT; i++)
	{
		TEST_NZ (sock_sync_data (ctx[i].client_sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		TEST_NZ(destroy_context(&ctx[i]));
	}
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
	if (argc != 1) {
		RDMAClient::usage(argv[0]);
		return 1;
	}
	
	RDMAClient client;
	client.start_client();
	return 0;
	
}