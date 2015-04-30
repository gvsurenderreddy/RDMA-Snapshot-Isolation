/*
 *	RDMAClient.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: erfanz
 */

#include "RDMAClient.hpp"
#include "../util/utils.hpp"
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
RDMAClient::Cart			RDMAClient::cart;
RDMAClient::SharedContext	RDMAClient::shared_context;	

/*
int RDMAClient::create_ts_context (struct TimestampRDMAClientContext *ts_ctx) {
	TEST_NZ(RDMACommon::build_connection(ts_ctx->ib_port, &ts_ctx->ib_ctx, &ts_ctx->port_attr, &ts_ctx->pd, &ts_ctx->cq, 10));
	TEST_NZ(register_ts_memory(ts_ctx));
	TEST_NZ(RDMACommon::create_queuepair(ts_ctx->ib_ctx, ts_ctx->pd, ts_ctx->cq, &(ts_ctx->qp)));
	return 0;
}
*/

/*
int RDMAClient::register_ts_memory(TimestampRDMAClientContext *ts_ctx) {
	int mr_flags = IBV_ACCESS_LOCAL_WRITE;
	
	TEST_Z(ts_ctx->timestamp_req_mr	= ibv_reg_mr(ts_ctx->pd, &ts_ctx->timestamp_request, sizeof(struct TimestampRequest), mr_flags));
	TEST_Z(ts_ctx->timestamp_res_mr	= ibv_reg_mr(ts_ctx->pd, &ts_ctx->timestamp_response, sizeof(struct TimestampResponse), mr_flags));
	return 0;
}
*/

void RDMAClient::usage (const char *argv0) {
	std::cout << "Usage:" << std::endl;
	std::cout << argv0 << " connects to server(s) specified in the config file" << std::endl;
}

int RDMAClient::check_if_version_is_in_block(int version, ItemVersion *items_region, int size) {
	int biggest_eligible_index = -1;
	int i;
	for (i=0; i < size; i++) {
		if (items_region[i].write_timestamp <= version) {
			// TODO: Ensure the validity of this section
			biggest_eligible_index = i;
			break;
		}
	}
	// TODO WRONGGGG
	biggest_eligible_index = 0;
	
	return biggest_eligible_index;
}

void RDMAClient::fill_shopping_cart(RDMAClientContext *ctx) {
	int item_id;
	int quantity;
	
	DEBUG_COUT ("[Info] Step 0: Cart contents: (Item ID,  Quantity)");
	for (int i=0; i < ORDERLINE_PER_ORDER; i++) {
		item_id		= (i * ITEM_PER_SERVER) + (rand() % ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		quantity	= (rand() % 5) + 1;			// the quanity of the item (not importatn)
		RDMAClient::cart.cart_lines[i].SCL_I_ID	= item_id;
		RDMAClient::cart.cart_lines[i].SCL_QTY	= quantity;
		DEBUG_COUT (".... " <<  item_id << '\t' << quantity);
	
		ctx[i].associated_cart_line = &RDMAClient::cart.cart_lines[i];
	}
}

int RDMAClient::acquire_read_timestamp(RDMAClientContext *ctx, uint64_t *read_timestamp) {
	// the first server is responsible for timestamps (timestamp oracle)
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
	ctx[0].qp,
	ctx[0].read_ts_mr,
	(uint64_t)&ctx[0].read_ts_region,
	&(ctx[0].peer_mr_timestamp),
	(uint64_t)ctx[0].peer_mr_timestamp.addr,
	(uint32_t)sizeof(uint64_t),
	true));
	TEST_NZ (RDMACommon::poll_completion(ctx[0].cq));
	
	*read_timestamp = ctx[0].read_ts_region.value;
	return 0;
}

void* RDMAClient::get_item_info(RDMAClientContext &ctx) {
	int fetch_block_num = 0;
	int item_id = ctx.associated_cart_line->SCL_I_ID;
	
		// Find the lookup address on the server remote memory 
	int item_offset = (item_id * MAX_ITEM_VERSIONS * sizeof(ItemVersion))
		+ fetch_block_num * FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	
	ItemVersion *item_lookup_address =  (ItemVersion *)(item_offset + ((uint64_t)ctx.peer_mr_items.addr));

	// RDMA READ it from the server
	TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
	ctx.qp,
	ctx.items_mr,
	(uint64_t)ctx.items_region,
	&(ctx.peer_mr_items),
	(uint64_t)item_lookup_address,
	FETCH_BLOCK_SIZE * sizeof(ItemVersion),
	true));
}

int RDMAClient::acquire_commit_timestamp(RDMAClientContext &ctx, uint64_t *commit_timestamp) {
	TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(ctx.qp,
	ctx.commit_ts_mr,
	(uint64_t)&ctx.commit_ts_region,
	&(ctx.peer_mr_timestamp),
	(uint64_t)ctx.peer_mr_timestamp.addr,
	(uint64_t)1ULL,
	(uint32_t)sizeof(Timestamp)));
	TEST_NZ (RDMACommon::poll_completion(ctx.cq));
	
	// Byte swapping, since all values read by atomic operations are in reverse bit order
	ctx.commit_ts_region.value = hton64(ctx.commit_ts_region.value);
	ctx.commit_ts_region.value += 1ULL;
	*commit_timestamp = ctx.commit_ts_region.value;
	return 0;
	
	/*
	TEST_NZ (RDMACommon::post_RECEIVE (ts_ctx->qp, ts_ctx->timestamp_res_mr, (uintptr_t)&ts_ctx->timestamp_response, sizeof(struct TimestampResponse)));
	TEST_NZ (RDMACommon::post_SEND (ts_ctx->qp, ts_ctx->timestamp_req_mr, (uintptr_t)&ts_ctx->timestamp_request, sizeof(struct TimestampRequest), true));
	TEST_NZ (RDMACommon::poll_completion (ts_ctx->cq));		// completion for post_SEND
	DEBUG_COUT("[Sent] Timestamp request to TS-Server");
	
	TEST_NZ (RDMACommon::poll_completion (ts_ctx->cq));		// completion for post_RECEIVE
	DEBUG_COUT("[Recv] Timestamp response from TS-Server " << ts_ctx->timestamp_response.commit_timestamp.value);
	*commit_timestamp = ts_ctx->timestamp_response.commit_timestamp.value;
	*/
}

void* RDMAClient::acquire_item_lock(RDMAClientContext &ctx, uint64_t *expected_lock, uint64_t *new_lock) {
	int item_id = ctx.associated_cart_line->SCL_I_ID;
	
	// and the actual content of the lock (before being swapped) will be stored in the following variable
	memset(ctx.lock_items_region, 0, sizeof(uint64_t));
	
	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx.peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(ctx.qp,
	ctx.lock_items_mr,
	(uint64_t)ctx.lock_items_region,
	&(ctx.peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	(uint64_t)expected_lock,
	(uint64_t)new_lock));
}

void* RDMAClient::revert_lock(RDMAClientContext &ctx, uint64_t *new_lock) {
	int item_id		= ctx.associated_cart_line->SCL_I_ID;
	
	memcpy(ctx.lock_items_region, new_lock, sizeof(uint64_t));	
		
	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx.peer_mr_lock_items.addr));
		
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx.qp,
	ctx.lock_items_mr,
	(uint64_t)ctx.lock_items_region,
	&(ctx.peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	true));
}


void* RDMAClient::decrement_item_stock(RDMAClientContext &ctx) {
	int item_id		= ctx.associated_cart_line->SCL_I_ID;
	int quantity	= ctx.associated_cart_line->SCL_QTY;
	
	int fetch_block_num = 0;
	
	ctx.items_region[0].write_timestamp = shared_context.commit_timestamp;
	ctx.items_region[0].item.I_STOCK 	-= quantity;
	
	if (ctx.items_region[0].item.I_STOCK < 10)
		ctx.items_region[0].item.I_STOCK += 20;
	
	// Find the lookup address on the server remote memory 
	int item_offset = (item_id * MAX_ITEM_VERSIONS * sizeof(ItemVersion))
		+ fetch_block_num * FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	
	ItemVersion *item_lookup_address =  (ItemVersion *)(item_offset + ((uint64_t)ctx.peer_mr_items.addr));
	
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx.qp,
	ctx.items_mr,
	(uint64_t)ctx.items_region,
	&(ctx.peer_mr_items),
	(uint64_t)item_lookup_address,
	(uint32_t)sizeof(ItemVersion),
	false));

	DEBUG_COUT (".... Successfully decremented the stock for item " << item_id
		<< " (with commit timestamp " << shared_context.commit_timestamp << ")");
}

void* RDMAClient::release_lock(RDMAClientContext &ctx) {
	int item_id		= ctx.associated_cart_line->SCL_I_ID;
	uint32_t		lock_status, version;
	
	lock_status		= (uint32_t) 0;
	version			= (uint32_t) shared_context.commit_timestamp;
	//version			= (uint32_t) 0;
	
	ctx.lock_items_region[0] = Lock::set_lock(lock_status, version);	

	int lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_address	= (uint64_t *)(lock_offset + ((uint64_t)ctx.peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ctx.qp,
	ctx.lock_items_mr,
	(uint64_t)ctx.lock_items_region,
	&(ctx.peer_mr_lock_items),
	(uint64_t)lock_lookup_address,
	(uint32_t)sizeof(uint64_t),
	true));
}

int RDMAClient::start_transaction(RDMAClientContext *ctx) {
	int		server_num;
	int		abort_cnt = 0;
	bool	abort_flag;
    bool	successful_locking_servers[SERVER_CNT] = { false };		// initializes the entire array to false 
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock;
	double avg_read_ts = 0, avg_fetch_info = 0, avg_commit_ts = 0, avg_lock = 0, avg_decrement = 0, avg_unlock = 0;
	
	uint32_t	lock_status, version;
	uint64_t	expected_lock[SERVER_CNT], new_lock[SERVER_CNT];
	
	DEBUG_COUT ("Transaction now gets started");
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer	
	for (int trx_num = 1; trx_num <= TRANSACTION_CNT; trx_num++){
		
		// ************************************************
		//	Acquiring read timestamp
		// ************************************************
		DEBUG_COUT (std::endl << "[Info] Handling transaction #" << trx_num);
		fill_shopping_cart(ctx);
		
		clock_gettime(CLOCK_REALTIME, &before_read_ts);
		
		TEST_NZ (acquire_read_timestamp(ctx, &shared_context.read_timestamp));
		DEBUG_COUT ("[Read] Step 1: Received read timestamp from oracle with TID " << shared_context.read_timestamp);
		
		clock_gettime(CLOCK_REALTIME, &after_read_ts);
	
	
		// ************************************************
		//	Fetching ITEMs information
		// ************************************************
		// TODO: Should be fixed. multiple requests to the same server should be sent with only one signalled request 
		for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
			// first find which server the data corresponds to
			server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
			get_item_info(ctx[server_num]);
		}
		for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
			server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
			TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
			DEBUG_COUT ("[Read] Step 2-" << i << ": Received information for item " << ctx[server_num].items_region->item.I_ID
				<< " from " << ctx[server_num].server_address);
		}
		DEBUG_COUT ("[Info] Step 2: Received information for all items");	
		
		clock_gettime(CLOCK_REALTIME, &after_fetch_info);
		
		
		// Since we only have one version per item (and hence one version is read for each item), we simplify this section
		// int found_version_index = RDMAClient::check_if_version_is_in_block(ctx->read_ts_region[0].timestamp,
		// ctx->items_region, MAX_ITEM_VERSIONS);
		// if (found_version_index >= 0)
		
		int found_version_index = 0;
		if (true) {
			// Found the version in the received block
			// DEBUG_COUT("....... Version Found in block " << ctx[0].fetch_block_num << " with timestamp "
			// << ctx[0].items_region[found_version_index].write_timestamp);
				
			// ************************************************
			//	Commit begins
			// ************************************************	
			// ************************************************
			//	Acquiring commit timestamp
			acquire_commit_timestamp(ctx[0], &shared_context.commit_timestamp);	
			DEBUG_COUT ("[FTCH] Step 3: Acquired commit timestamp " << shared_context.commit_timestamp);	
			clock_gettime(CLOCK_REALTIME, &after_commit_ts);
			
			
			// ************************************************
			// First step, acquiring locks
			for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
				server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;

				// lock expected before locking 
				lock_status					= (uint32_t) 0;
				version						= (uint32_t) ctx[server_num].items_region[found_version_index].write_timestamp;
				//version						= (uint32_t) 0;
				expected_lock[server_num]	= Lock::set_lock(lock_status, version);

				// new lock
				lock_status					= (uint32_t) 1;
				version						= (uint32_t) shared_context.commit_timestamp;
				//version						= (uint32_t) 0;			
				new_lock[server_num]		= Lock::set_lock(lock_status, version);
			
				acquire_item_lock(ctx[server_num], &expected_lock[server_num], &new_lock[server_num]);
			}
		
			abort_flag = false;
			for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
				server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
			
				TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));

				// Byte swapping, since all values read by atomic operations are in reverse bit order
				ctx[server_num].lock_items_region[0]	= hton64(ctx[server_num].lock_items_region[0]);

				// Check if Compare-and-swaping the lock was successful
				if (Lock::are_equals(ctx[server_num].lock_items_region[0], expected_lock[server_num])) {
					DEBUG_COUT ("[CMSW] Step 4-" << i << ": Lock on item " << cart.cart_lines[i].SCL_I_ID << " successfully acquired.");
					successful_locking_servers[server_num] = true;
				}
				else {
					DEBUG_COUT ("[CMSW] Step 4-" << i << ": Failed to lock item " << cart.cart_lines[i].SCL_I_ID);
					DEBUG_COUT ("........ Expected lock: " << Lock::get_lock_status(expected_lock[server_num])
						<< " | " << Lock::get_version(expected_lock[server_num]));
					DEBUG_COUT ("........ Existing lock: " << Lock::get_lock_status(ctx[server_num].lock_items_region[0])
						<< " | " << Lock::get_version(ctx[server_num].lock_items_region[0]));
			
					successful_locking_servers[server_num] = false;					
					abort_flag = true;
				}	
			}
		
			// Check if all locks are successfully acquired 
			if (abort_flag == true) {
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true)
						revert_lock(ctx[server_num], &expected_lock[server_num]);
				}
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true) {
						TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
						DEBUG_COUT ("[Writ] Step 4: Lock reverted on item " << ctx[server_num].items_region->item.I_ID);
					}
				}
				DEBUG_CERR ("[Info] Step 4: Lock on all items could not be acquired :(");
				abort_cnt++;
				continue;
			}
			else {
				clock_gettime(CLOCK_REALTIME, &after_lock);	
				DEBUG_COUT ("[Info] Step 4: All locks successfully acquired");
		
				// ************************************************
				//	Decrementing the stock in ITEM table
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					decrement_item_stock(ctx[server_num]);
				}				
				DEBUG_COUT ("[Writ] Step 5: Successfully decremented the stock in table ITEM (unsignaled)");
				clock_gettime(CLOCK_REALTIME, &after_decrement);
		
			
				// ************************************************
				//	Releasing the lock
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
					release_lock(ctx[server_num]);
				}
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					server_num = cart.cart_lines[i].SCL_I_ID / ITEM_PER_SERVER;
				
					TEST_NZ (RDMACommon::poll_completion(ctx[server_num].cq));
					DEBUG_COUT ("[Writ] Lock on item " << cart.cart_lines[i].SCL_I_ID 
						<< " released with commit timestamp " << shared_context.commit_timestamp);
				}					
				DEBUG_COUT ("[Info] Step 6: All locks successfully released");
				clock_gettime(CLOCK_REALTIME, &after_unlock);
				
				double t = ( after_read_ts.tv_sec - before_read_ts.tv_sec ) * 1E9 + ( after_read_ts.tv_nsec - before_read_ts.tv_nsec );
				avg_read_ts += t;
				
				t = ( after_fetch_info.tv_sec - after_read_ts.tv_sec ) * 1E9 + ( after_fetch_info.tv_nsec - after_read_ts.tv_nsec );
				avg_fetch_info += t;
				
				t = ( after_commit_ts.tv_sec - after_fetch_info.tv_sec ) * 1E9 + ( after_commit_ts.tv_nsec - after_fetch_info.tv_nsec );
				avg_commit_ts += t;
				
				t = ( after_lock.tv_sec - after_commit_ts.tv_sec ) * 1E9 + ( after_lock.tv_nsec - after_commit_ts.tv_nsec );
				avg_lock += t;
				
				t = ( after_decrement.tv_sec - after_lock.tv_sec ) * 1E9 + ( after_decrement.tv_nsec - after_lock.tv_nsec );
				avg_decrement += t;
				
				t = ( after_unlock.tv_sec - after_decrement.tv_sec ) * 1E9 + ( after_unlock.tv_nsec - after_decrement.tv_nsec );
				avg_unlock += t;
				
				
				/*
				// ************************************************
				//	Add a new record to table ORDERS (which is stored on only one server)
				ctx[0].orders_region.write_timestamp	= shared_context.commit_timestamp;
				ctx[0].orders_region.orders.O_ID		= shared_context.commit_timestamp;
			
				//int order_offset = ((ctx[0].orders_region.orders.O_ID - 1) * sizeof(OrdersVersion));	// TODO
				int order_offset = ( ( (ctx[0].orders_region.orders.O_ID - 1) % MAX_BUFFER_SIZE) * sizeof(OrdersVersion));
				OrdersVersion *orders_lookup_address =  (OrdersVersion *)(order_offset + (uint64_t)ctx[0].peer_mr_orders.addr);
		
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].orders_mr,
				(uint64_t)&ctx[0].orders_region,
				&(ctx[0].peer_mr_orders),
				(uint64_t)orders_lookup_address,
				(uint32_t)sizeof(OrdersVersion),
				false));
				DEBUG_COUT ("[Writ] Step 7: Successfully added a new record to table ORDERS (unsignaled)");
			
			
				// ************************************************
				//	Add new record(s) to table ORDERLINE (which is stored on only one server)
				for (int i = 0; i < ORDERLINE_PER_ORDER; i++) {
					ctx[0].order_line_region[i].write_timestamp		= shared_context.commit_timestamp;
					ctx[0].order_line_region[i].order_line.OL_ID		= (shared_context.commit_timestamp - 1) * ORDERLINE_PER_ORDER + 1 + i;
					ctx[0].order_line_region[i].order_line.OL_O_ID	= ctx[0].orders_region.orders.O_ID;
					ctx[0].order_line_region[i].order_line.OL_I_ID	= cart.cart_lines[i].SCL_I_ID;
					ctx[0].order_line_region[i].order_line.OL_QTY		= cart.cart_lines[i].SCL_QTY;
				}
				//int ol_offset = (shared_context.commit_timestamp - 1) * ORDERLINE_PER_ORDER * sizeof(OrderLineVersion);	// TODO
				int ol_offset = (((shared_context.commit_timestamp - 1) * ORDERLINE_PER_ORDER) % MAX_BUFFER_SIZE) * sizeof(OrderLineVersion);
				OrderLineVersion *ol_lookup_address = (OrderLineVersion *)(ol_offset + (uint64_t)ctx[0].peer_mr_order_line.addr);
				
				
		
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].order_line_mr,
				(uint64_t)ctx[0].order_line_region,
				&(ctx[0].peer_mr_order_line),
				(uint64_t)ol_lookup_address,
				(uint32_t)ORDERLINE_PER_ORDER * sizeof(OrderLineVersion),
				false));
				DEBUG_COUT ("[Writ] Step 8: Successfully added new record(s) to table ORDERLINE (unsignaled)");
	
			
				// ************************************************
				//	Adding a new record to CC_XACTS table (which is stored on only one server)
				ctx[0].cc_xacts_region.write_timestamp = shared_context.commit_timestamp;
				ctx[0].cc_xacts_region.cc_xacts.CX_O_ID = ctx[0].orders_region.orders.O_ID;
	
				// int cc_offset = ((shared_context.commit_timestamp - 1) * sizeof(CCXactsVersion));
				int cc_offset = ( ( (shared_context.commit_timestamp - 1) % MAX_BUFFER_SIZE) * sizeof(CCXactsVersion));	
				CCXactsVersion *cc_lookup_address =  (CCXactsVersion *)(cc_offset + (uint64_t)ctx[0].peer_mr_cc_xacts.addr);
		
				TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ctx[0].qp,
				ctx[0].cc_xacts_mr,
				(uint64_t)&ctx[0].cc_xacts_region,
				&(ctx[0].peer_mr_cc_xacts),
				(uint64_t)cc_lookup_address,
				(uint32_t)sizeof(CCXactsVersion),
				true));
				TEST_NZ (RDMACommon::poll_completion(ctx[0].cq));
				DEBUG_COUT ("[Writ] Step 9: Successfully added a new record to table CC_XACTS (signaled)");
		
				DEBUG_COUT ("[Info] Transaction finished successfully!");
				*/
			}	
		}
	}
	
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer
	
	double micro_elapsed_time = ( ( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + ( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	double T_P_MILISEC = (double)(TRANSACTION_CNT / (double)(micro_elapsed_time / 1000));
	
	avg_read_ts /= 1000;
	avg_fetch_info /= 1000;
	avg_commit_ts /= 1000;
	avg_lock /= 1000;
	avg_decrement /= 1000;
	avg_unlock /= 1000;
	
	int committed_cnt = TRANSACTION_CNT - abort_cnt;
	double success_rate = (double)committed_cnt /  TRANSACTION_CNT;
	
	std::cout << "[Stat] Avg read ts (u sec):	" << (double)avg_read_ts / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg fetch (u sec): 	" << (double)avg_fetch_info / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg commit ts (u sec):	" << (double)avg_commit_ts / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg lock (u sec):  	" << (double)avg_lock / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg decrement (u sec):	" << (double)avg_decrement / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg unlock (u sec):	" << (double)avg_unlock / committed_cnt << std::endl; 
	std::cout << "[Stat] Avg cumulative(u sec):	" << (double)micro_elapsed_time / TRANSACTION_CNT << std::endl; 
	std::cout << "[Stat] " << committed_cnt << " committed, " << abort_cnt << " aborted (success rate = " << success_rate << ")." << std::endl;
	std::cout << "[Stat] Transaction per millisec:	" <<  T_P_MILISEC << std::endl;
	return 0;
}

int RDMAClient::start_client () {	
	// Init RDMAClientContext
	RDMAClientContext ctx[SERVER_CNT];
	//struct TimestampRDMAClientContext ts_ctx;
	char temp_char;
	
	srand (generate_random_seed());		// initialize random seed
    
   	/*
	// First, connect to Timestamp Server
	strcpy(ts_ctx.server_address, TIMESTAMP_SERVER_ADDR.c_str());
	ts_ctx.tcp_port	= TIMESTAMP_SERVER_PORT;
	ts_ctx.ib_port	= TIMESTAMP_SERVER_IB_PORT;
		
	TEST_NZ (establish_tcp_connection(ts_ctx.server_address, ts_ctx.tcp_port, &ts_ctx.timestamp_sockfd));
	
	DEBUG_COUT("[Conn] Connection established to Timestamp Server");

	TEST_NZ (create_ts_context (&ts_ctx));

	TEST_NZ (RDMACommon::connect_qp (&(ts_ctx.qp), ts_ctx.ib_port, ts_ctx.port_attr.lid, ts_ctx.timestamp_sockfd));
	DEBUG_COUT("[Conn] QPed to Timestamp Server");
	*/
		
	// then, client connext to servers
	for (int i = 0; i < SERVER_CNT; i++){
	    ctx[i].server_address = "";
		ctx[i].server_address += SERVER_ADDR[i];
		ctx[i].ib_port		  = IB_PORT[i];
		
		
		// Connect to servers
		TEST_NZ (establish_tcp_connection(ctx[i].server_address, TCP_PORT[i], &ctx[i].sockfd));
		DEBUG_COUT("[Conn] Connection established to server " << i);
	
		TEST_NZ (ctx[i].create_context());
		
		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(ctx[i].qp, ctx[i].recv_mr, (uintptr_t)&ctx[i].recv_msg, sizeof(struct MemoryKeys));
	
		TEST_NZ (RDMACommon::connect_qp (&(ctx[i].qp), ctx[i].ib_port, ctx[i].port_attr.lid, ctx[i].sockfd));
		DEBUG_COUT("[Conn] QPed to server " << i);		
	
		TEST_NZ(RDMACommon::poll_completion(ctx[i].cq));
		DEBUG_COUT("[Recv] buffers info from server " << i);
	
		// after receiving the message from the server, let's store its addresses in the context
		memcpy(&ctx[i].peer_mr_items,		&ctx[i].recv_msg.mr_items,		sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_orders,		&ctx[i].recv_msg.mr_orders,		sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_order_line,	&ctx[i].recv_msg.mr_order_line,	sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_cc_xacts,	&ctx[i].recv_msg.mr_cc_xacts,	sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_timestamp,	&ctx[i].recv_msg.mr_timestamp,	sizeof(struct ibv_mr));
		memcpy(&ctx[i].peer_mr_lock_items,	&ctx[i].recv_msg.mr_lock_items,	sizeof(struct ibv_mr));
	}
	DEBUG_COUT ("[Info] Successfully connected to all servers");
	
	start_transaction(ctx);
	
	DEBUG_COUT("[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ctx[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT("[Conn] Notified server " << i << " it's done");
		TEST_NZ ( ctx[i].destroy_context());
	}
}

int main (int argc, char *argv[]) {
	if (argc != 1) {
		RDMAClient::usage(argv[0]);
		return 1;
	}
	RDMAClient client;
	client.start_client();
	return 0;
}