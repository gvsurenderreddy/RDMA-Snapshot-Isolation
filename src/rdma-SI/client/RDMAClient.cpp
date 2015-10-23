/*
 *	RDMAClient.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#include "RDMAClient.hpp"

#include "../../util/utils.hpp"
#include "../../util/zipf.hpp"
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

#define CLASS_NAME	"RDMAClient"

void RDMAClient::fill_shopping_cart() {
	int item_id;
	int quantity;
	
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 0: Cart contents: (Item ID,  Quantity)");
	for (int i=0; i < config::ORDERLINE_PER_ORDER; i++) {
		item_id		= (i * config::ITEM_PER_SERVER) + (rand() % config::ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		// item_id		= (i * ITEM_PER_SERVER) + zipf_get_sample(SKEWNESS_IN_ITEM_ACCESS, ITEM_PER_SERVER) - 1;	// generating in the range 0 to ITEM_CNT
		quantity	= (rand() % 5) + 1;			// the quantity of the item (not important)
		cart_.cart_lines[i].SCL_I_ID	= item_id;
		cart_.cart_lines[i].SCL_QTY	= quantity;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] ... " <<  item_id << '\t' << quantity);
	
		ds_ctx_[i].associated_cart_line = &cart_.cart_lines[i];
	}
}

int RDMAClient::acquire_read_ts() {
	// the first server is responsible for timestamps (timestamp oracle)
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
	ts_ctx_.qp,
	ts_ctx_.mr_read_timestamp,
	(uint64_t)&ts_ctx_.read_timestamp,
	&(ts_ctx_.peer_mr_read_timestamp),
	(uint64_t)ts_ctx_.peer_mr_read_timestamp.addr,
	(uint32_t)sizeof(Timestamp),
	true));
	TEST_NZ (RDMACommon::poll_completion(ts_ctx_.send_cq));
	return 0;
}

int RDMAClient::get_item_info(const size_t server_num) {
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	
	// Find the lookup address on the server remote memory
	size_t item_offset = (size_t)(item_id * config::MAX_ITEM_VERSIONS * sizeof(ItemVersion));
	
	ItemVersion *item_lookup_address =  (ItemVersion *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items.addr));

	// RDMA READ it from the server
	TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
	ds_ctx_[server_num].qp,
	ds_ctx_[server_num].mr_items,
	(uint64_t)ds_ctx_[server_num].items_region,
	&(ds_ctx_[server_num].peer_mr_items),
	(uint64_t)item_lookup_address,
	(uint32_t)(config::FETCH_BLOCK_SIZE * sizeof(ItemVersion)),
	true));
	return 0;
}

int RDMAClient::acquire_commit_ts() {
	TEST_NZ (RDMACommon::post_RDMA_FETCH_ADD(
	ts_ctx_.qp,
	ts_ctx_.mr_commit_timestamp,
	(uint64_t)&ts_ctx_.commit_timestamp,
	&(ts_ctx_.peer_mr_commit_timestamp),
	(uint64_t)ts_ctx_.peer_mr_commit_timestamp.addr,
	(uint64_t)1ULL,
	(uint32_t)sizeof(Timestamp)));
	TEST_NZ (RDMACommon::poll_completion(ts_ctx_.send_cq));
	
	// Byte swapping, since values read by atomic operations are in Big Endian order
	ts_ctx_.commit_timestamp.value = bigEndianToHost(ts_ctx_.commit_timestamp.value);
	return 0;
}

int RDMAClient::acquire_item_lock(const size_t server_num, uint64_t *expected_lock, uint64_t *new_lock) {
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	
	// and the actual content of the lock (before being swapped) will be stored in the following variable
	memset(ds_ctx_[server_num].lock_items_region, 0, sizeof(uint64_t));
	
	size_t lock_offset			= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_addr	= (uint64_t *)(lock_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(ds_ctx_[server_num].qp,
	ds_ctx_[server_num].mr_lock_items,
	(uint64_t)ds_ctx_[server_num].lock_items_region,
	&(ds_ctx_[server_num].peer_mr_lock_items),
	(uint64_t)lock_lookup_addr,
	(uint32_t)sizeof(uint64_t),
	(uint64_t)expected_lock,
	(uint64_t)new_lock));
	
	return 0;
}

int RDMAClient::revert_lock(const size_t server_num, uint64_t *new_lock) {
	int item_id		= ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	
	memcpy(ds_ctx_[server_num].lock_items_region, new_lock, sizeof(uint64_t));
		
	size_t lock_offset			= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_addr	= (uint64_t *)(lock_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_lock_items.addr));
		
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ds_ctx_[server_num].qp,
	ds_ctx_[server_num].mr_lock_items,
	(uint64_t)ds_ctx_[server_num].lock_items_region,
	&(ds_ctx_[server_num].peer_mr_lock_items),
	(uint64_t)lock_lookup_addr,
	(uint32_t)sizeof(uint64_t),
	true));
	
	return 0;
}


int RDMAClient::update_item_stock(const size_t server_num) {
	int item_id		= ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	int quantity	= ds_ctx_[server_num].associated_cart_line->SCL_QTY;
	
	int fetch_block_num = 0;
	
	ds_ctx_[server_num].items_region[0].write_timestamp.value = ts_ctx_.commit_timestamp.value;
	ds_ctx_[server_num].items_region[0].item.I_STOCK 	-= quantity;
	
	if (ds_ctx_[server_num].items_region[0].item.I_STOCK < 10)
		ds_ctx_[server_num].items_region[0].item.I_STOCK += 20;
	
	// Find the lookup address on the server remote memory 
	size_t item_offset = (item_id * config::MAX_ITEM_VERSIONS * sizeof(ItemVersion))
		+ fetch_block_num * config::FETCH_BLOCK_SIZE * sizeof(ItemVersion);
	
	ItemVersion *item_lookup_addr =  (ItemVersion *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items.addr));
	
	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ds_ctx_[server_num].qp,
	ds_ctx_[server_num].mr_items,
	(uint64_t)ds_ctx_[server_num].items_region,
	&(ds_ctx_[server_num].peer_mr_items),
	(uint64_t)item_lookup_addr,
	(uint32_t)sizeof(ItemVersion),
	false));

	DEBUG_COUT (CLASS_NAME, __func__, "[WRIT] ... Successfully updated the stock for item " << item_id
		<< " (with commit timestamp " << ts_ctx_.commit_timestamp.value << ")");
	
	return 0;
}

int RDMAClient::release_lock(const size_t server_num) {
	int item_id		= ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	uint32_t		lock_status, version;
	
	lock_status		= (uint32_t) 0;
	version			= (uint32_t) ts_ctx_.commit_timestamp.value;
	//version			= (uint32_t) 0;
	
	ds_ctx_[server_num].lock_items_region[0] = Lock::set_lock(lock_status, version);

	size_t lock_offset					= item_id * sizeof(uint64_t);
	uint64_t *lock_lookup_addr	= (uint64_t *)(lock_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_lock_items.addr));

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
	ds_ctx_[server_num].qp,
	ds_ctx_[server_num].mr_lock_items,
	(uint64_t)ds_ctx_[server_num].lock_items_region,
	&(ds_ctx_[server_num].peer_mr_lock_items),
	(uint64_t)lock_lookup_addr,
	(uint32_t)sizeof(uint64_t),
	true));
	
	return 0;
}

int RDMAClient::submit_trx_result(Result result, int transactionNum) {
	bool signaled = false;
	// if (transactionNum % 5000 == 0)
	//	signaled = true;
	signaled = true;

	ts_ctx_.trx_status = 1;

	size_t offset			= (ts_ctx_.commit_timestamp.value * sizeof(uint8_t)) % config::TIMESTAMP_SERVER_QUEUE_SIZE;
	uint64_t *write_addr	= (uint64_t *)(offset + (uint64_t)ts_ctx_.peer_mr_finished_trxs_hash.addr);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
		ts_ctx_.qp,
		ts_ctx_.mr_trx_status,
		(uint64_t)&ts_ctx_.trx_status,
		&(ts_ctx_.peer_mr_finished_trxs_hash),
		(uint64_t)write_addr,
		(uint32_t)sizeof(uint8_t),
		signaled));

	if (signaled)
		TEST_NZ (RDMACommon::poll_completion(ts_ctx_.send_cq));

	DEBUG_COUT (CLASS_NAME, __func__, "[Sent] Transaction result for CTS " << ts_ctx_.commit_timestamp.value << " sent to the TS server");
	return 0;
}

int RDMAClient::startTransactions() {
	int		server_num;
	int		abort_cnt = 0;
	bool	abort_flag;
    bool	successful_locking_servers[config::SERVER_CNT] = { false };		// initializes the entire array to false
	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_decrement, after_unlock, after_result_submission;
	double avg_read_ts = 0, avg_fetch_info = 0, avg_commit_ts = 0, avg_lock = 0, avg_decrement = 0, avg_unlock = 0, avg_result_submission = 0;
	
	uint32_t	lock_status, version;
	uint64_t	expected_lock[config::SERVER_CNT], new_lock[config::SERVER_CNT];
	
	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer	
	for (int trx_num = 1; trx_num <= config::TRANSACTION_CNT; trx_num++){
		//if (trx_num % 100 == 0)
		//	std::cout << "Handling trx #" << trx_num << std::endl;

		DEBUG_COUT (CLASS_NAME, __func__, std::endl << "[Info] Handling transaction #" << trx_num);


		// ************************************************
		//	Acquire read timestamp
		// ************************************************
		fill_shopping_cart();
		
		clock_gettime(CLOCK_REALTIME, &before_read_ts);
		
		TEST_NZ (acquire_read_ts());
		DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 1: Received read timestamp from oracle with TID " << ts_ctx_.read_timestamp.value);
		
		clock_gettime(CLOCK_REALTIME, &after_read_ts);
	
	
		// ************************************************
		//	Read records in read-set (fetch ITEMs information
		// ************************************************
		// TODO: Should be fixed. multiple requests to the same server should be sent with only one signalled request 


		// TODO: BIGGGGGG TODOOOOOOOOOOO: Check whether this is the version you need.
		for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
			// first find which server the data corresponds to
			server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
			TEST_NZ(get_item_info(server_num));
		}
		for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
			server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
			TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
			DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 2-" << i << ": Received information for item " << ds_ctx_[server_num].items_region->item.I_ID
					<< "(ts: " << ds_ctx_[server_num].items_region->write_timestamp.value << ") from " << ds_ctx_[server_num].server_address);
		}
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 2: Received information for all items");
		clock_gettime(CLOCK_REALTIME, &after_fetch_info);
		
		
		// Since we only have one version per item (and hence one version is read for each item), we simplify this section
		// int found_version_index = RDMAClient::check_if_version_is_in_block(ctx->read_ts_region[0].timestamp,
		// ctx->items_region, MAX_ITEM_VERSIONS);
		// if (found_version_index >= 0)
		
		int found_version_index = 0;
		if (true) {
			// Found the version in the received block
				
			// ************************************************
			//	Acquire Commit timestamp
			// ************************************************	
			acquire_commit_ts();
			DEBUG_COUT (CLASS_NAME, __func__, "[FTCH] Step 3: Acquired commit timestamp " << ts_ctx_.commit_timestamp.value);
			clock_gettime(CLOCK_REALTIME, &after_commit_ts);
			
			
			// ************************************************
			// Acquire locks for items in write-set
			// ************************************************
			for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
				server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;

				// lock expected before locking 
				lock_status					= (uint32_t) 0;
				version						= (uint32_t) ds_ctx_[server_num].items_region[found_version_index].write_timestamp.value;
				expected_lock[server_num]	= Lock::set_lock(lock_status, version);

				// new lock
				lock_status					= (uint32_t) 1;
				version						= (uint32_t) ts_ctx_.commit_timestamp.value;
				new_lock[server_num]		= Lock::set_lock(lock_status, version);

				acquire_item_lock(server_num, &expected_lock[server_num], &new_lock[server_num]);
			}
		
			abort_flag = false;
			for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
				server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));

				// Byte swapping, since values read by atomic operations are in Big Endian order
				ds_ctx_[server_num].lock_items_region[0] = bigEndianToHost(ds_ctx_[server_num].lock_items_region[0]);

				// Check if CASing the lock was successful
				if (Lock::are_equals(ds_ctx_[server_num].lock_items_region[0], expected_lock[server_num])) {
					DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Step 4-" << i << ": Lock on item " << cart.cart_lines[i].SCL_I_ID << " successfully acquired.");
					successful_locking_servers[server_num] = true;
				}
				else {
					DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Step 4-" << i << ": Failed to lock item " << cart.cart_lines[i].SCL_I_ID);
					DEBUG_COUT (CLASS_NAME, __func__, "........ Expected lock: " << Lock::get_lock_status(expected_lock[server_num])
						<< " | " << Lock::get_version(expected_lock[server_num]));
					DEBUG_COUT (CLASS_NAME, __func__, "........ Existing lock: " << Lock::get_lock_status(ds_ctx_[server_num].lock_items_region[0])
						<< " | " << Lock::get_version(ds_ctx_[server_num].lock_items_region[0]));
			
					successful_locking_servers[server_num] = false;					
					abort_flag = true;
				}	
			}
		
			// Check if all locks are successfully acquired 
			if (abort_flag == true) {
				for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
					server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true)
						TEST_NZ(revert_lock(server_num, &expected_lock[server_num]));
				}
				for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
					server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
					if (successful_locking_servers[server_num] == true) {
						TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
						DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Step 4: Lock reverted on item " << ds_ctx_[server_num].items_region->item.I_ID);
					}
				}
				DEBUG_CERR (CLASS_NAME, __func__, "[Info] Step 4: Lock on all items could not be acquired :(");
				abort_cnt++;
				submit_trx_result(message::TransactionResult::ABORTED, trx_num);
				continue;
			}
			else {
				clock_gettime(CLOCK_REALTIME, &after_lock);	
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 4: All locks successfully acquired");
		
				// ************************************************
				//	Update the stock in ITEM table
				// ************************************************
				for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
					server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
					update_item_stock(server_num);
				}				
				DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Step 5: Successfully updated the stock in table ITEM (unsignaled)");
				clock_gettime(CLOCK_REALTIME, &after_decrement);
		
			
				// ************************************************
				//	Install and unlock the new versions
				// ************************************************
				for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
					server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
					TEST_NZ(release_lock(server_num));
				}
				for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
					server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				
					TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
					DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Lock on item " << cart.cart_lines[i].SCL_I_ID
						<< " released with commit timestamp " << ts_ctx_.commit_timestamp.value);
				}
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 6: All locks successfully released");
				clock_gettime(CLOCK_REALTIME, &after_unlock);


				// ************************************************
				//	Submit the result to the timestamp server
				// ************************************************
				submit_trx_result(message::TransactionResult::COMMITTED, trx_num);
				clock_gettime(CLOCK_REALTIME, &after_result_submission);

				
				// ************************************************
				//	Update the stats
				// ************************************************
				double t = (double)( after_read_ts.tv_sec - before_read_ts.tv_sec ) * 1E9 + (double)( after_read_ts.tv_nsec - before_read_ts.tv_nsec );
				avg_read_ts += t;
				
				t = (double)( after_fetch_info.tv_sec - after_read_ts.tv_sec ) * 1E9 + (double)( after_fetch_info.tv_nsec - after_read_ts.tv_nsec );
				avg_fetch_info += t;
				
				t = (double)( after_commit_ts.tv_sec - after_fetch_info.tv_sec ) * 1E9 + (double)( after_commit_ts.tv_nsec - after_fetch_info.tv_nsec );
				avg_commit_ts += t;
				
				t = (double)( after_lock.tv_sec - after_commit_ts.tv_sec ) * 1E9 + (double)( after_lock.tv_nsec - after_commit_ts.tv_nsec );
				avg_lock += t;
				
				t = (double)( after_decrement.tv_sec - after_lock.tv_sec ) * 1E9 + (double)( after_decrement.tv_nsec - after_lock.tv_nsec );
				avg_decrement += t;
				
				t = (double)( after_unlock.tv_sec - after_decrement.tv_sec ) * 1E9 + (double)( after_unlock.tv_nsec - after_decrement.tv_nsec );
				avg_unlock += t;
				
				t = (double)( after_result_submission.tv_sec - after_unlock.tv_sec ) * 1E9 + (double)( after_result_submission.tv_nsec - after_unlock.tv_nsec );
				avg_result_submission += t;

				
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
	
	double micro_elapsed_time = ( (double)( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + (double)( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	double T_P_MILISEC = (double)(config::TRANSACTION_CNT / (double)(micro_elapsed_time / 1000));
	
	avg_read_ts /= 1000;
	avg_fetch_info /= 1000;
	avg_commit_ts /= 1000;
	avg_lock /= 1000;
	avg_decrement /= 1000;
	avg_unlock /= 1000;
	avg_result_submission /= 1000;

	
	int committed_cnt = config::TRANSACTION_CNT - abort_cnt;
	double success_rate = (double)committed_cnt / config:: TRANSACTION_CNT;
	
	std::cout << "[Stat] Avg read ts (u sec):	" << (double)avg_read_ts / committed_cnt << std::endl;
	std::cout << "[Stat] Avg fetch (u sec): 	" << (double)avg_fetch_info / committed_cnt << std::endl;
	std::cout << "[Stat] Avg commit ts (u sec):	" << (double)avg_commit_ts / committed_cnt << std::endl;
	std::cout << "[Stat] Avg lock (u sec):  	" << (double)avg_lock / committed_cnt << std::endl;
	std::cout << "[Stat] Avg decrement (u sec):	" << (double)avg_decrement / committed_cnt << std::endl;
	std::cout << "[Stat] Avg unlock (u sec):	" << (double)avg_unlock / committed_cnt << std::endl;
	std::cout << "[Stat] Avg res subm (u sec):	" << (double)avg_result_submission / committed_cnt << std::endl;
	std::cout << "[Stat] Avg cumulative(u sec):	" << (double)micro_elapsed_time / config::TRANSACTION_CNT << std::endl;

	std::cout << "[Stat] " << committed_cnt << " committed, " << abort_cnt << " aborted (success rate = " << success_rate << ")." << std::endl;
	std::cout << "[Stat] Transaction per millisec:	" <<  T_P_MILISEC << std::endl;
	return 0;
}

int RDMAClient::start_client () {	
	/************************************************
	 * First, connect to Timestamp Server
	 ************************************************/
	TEST_NZ (establish_tcp_connection(ts_ctx_.server_address, config::TIMESTAMP_SERVER_PORT, &ts_ctx_.sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Timestamp Server");
	TEST_NZ (ts_ctx_.create_context ());

	// before connecting the queue pairs, we post the RECEIVE job to be ready for the timestamp server's message containing its memory locations
	RDMACommon::post_RECEIVE(ts_ctx_.qp, ts_ctx_.mr_recv, (uintptr_t)&ts_ctx_.recv_msg, sizeof(struct message::TimestampServerMemoryKeys));

	TEST_NZ (RDMACommon::connect_qp (&(ts_ctx_.qp), ts_ctx_.ib_port, ts_ctx_.port_attr.lid, ts_ctx_.sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to Timestamp Server");


	TEST_NZ(RDMACommon::poll_completion(ts_ctx_.recv_cq));
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from timestamp server" );

	// after receiving the message from the server, let's store its addresses in the context
	memcpy(&ts_ctx_.peer_mr_finished_trxs_hash,	&ts_ctx_.recv_msg.mr_finished_trxs_hash,	sizeof(struct ibv_mr));
	memcpy(&ts_ctx_.peer_mr_read_timestamp,		&ts_ctx_.recv_msg.mr_read_timestamp,		sizeof(struct ibv_mr));
	memcpy(&ts_ctx_.peer_mr_commit_timestamp,	&ts_ctx_.recv_msg.mr_commit_timestamp,	sizeof(struct ibv_mr));
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Successfully connected to the timestamp server");

		
	/************************************************
	 * Then, connect to all servers
	 ************************************************/
	for (int i = 0; i < config::SERVER_CNT; i++){
		// Connect to servers
		TEST_NZ (establish_tcp_connection(ds_ctx_[i].server_address, config::TCP_PORT[i], &ds_ctx_[i].sockfd));
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server " << i);
	
		TEST_NZ (ds_ctx_[i].create_context());
		
		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(ds_ctx_[i].qp, ds_ctx_[i].mr_recv, (uintptr_t)&ds_ctx_[i].recv_msg, sizeof(struct message::DataServerMemoryKeys));
	
		TEST_NZ (RDMACommon::connect_qp (&(ds_ctx_[i].qp), ds_ctx_[i].ib_port, ds_ctx_[i].port_attr.lid, ds_ctx_[i].sockfd));
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server " << i);
	
		TEST_NZ(RDMACommon::poll_completion(ds_ctx_[i].recv_cq));
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from server " << i);
	
		// after receiving the message from the server, let's store its addresses in the context
		memcpy(&ds_ctx_[i].peer_mr_items,		&ds_ctx_[i].recv_msg.mr_items,		sizeof(struct ibv_mr));
		memcpy(&ds_ctx_[i].peer_mr_orders,		&ds_ctx_[i].recv_msg.mr_orders,		sizeof(struct ibv_mr));
		memcpy(&ds_ctx_[i].peer_mr_order_line,	&ds_ctx_[i].recv_msg.mr_order_line,	sizeof(struct ibv_mr));
		memcpy(&ds_ctx_[i].peer_mr_cc_xacts,		&ds_ctx_[i].recv_msg.mr_cc_xacts,	sizeof(struct ibv_mr));
		memcpy(&ds_ctx_[i].peer_mr_timestamp,	&ds_ctx_[i].recv_msg.mr_timestamp,	sizeof(struct ibv_mr));
		memcpy(&ds_ctx_[i].peer_mr_lock_items,	&ds_ctx_[i].recv_msg.mr_lock_items,	sizeof(struct ibv_mr));
	}
	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Successfully connected to all servers");
	
	startTransactions();
	
	char temp_char;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < config::SERVER_CNT; i++) {
		TEST_NZ (sock_sync_data (ds_ctx_[i].sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified server " << i << " it's done");
		TEST_NZ ( ds_ctx_[i].destroy_context());
	}
	TEST_NZ (sock_sync_data (ts_ctx_.sockfd, 1, "W", &temp_char));	/* just send a dummy char back and forth */
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified timestamp server it's done");
	return 0;
}

RDMAClient::RDMAClient() {
	srand ((unsigned int)generate_random_seed());		// initialize random seed
	zipf_initialize(config::SKEWNESS_IN_ITEM_ACCESS, config::ITEM_PER_SERVER);

	ts_ctx_.server_address = "";
	ts_ctx_.server_address += config::TIMESTAMP_SERVER_ADDR;
	ts_ctx_.ib_port	= config::TIMESTAMP_SERVER_IB_PORT;

	for (int i = 0; i < config::SERVER_CNT; i++){
		ds_ctx_[i].server_address = "";
		ds_ctx_[i].server_address += config::SERVER_ADDR[i];
		ds_ctx_[i].ib_port		  = config::IB_PORT[i];
	}
}
