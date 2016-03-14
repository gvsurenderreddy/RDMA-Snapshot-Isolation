/*
 *	RDMAClient.cpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#include "TPCW_Client.hpp"

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
#include <time.h>		// for struct timespec
#include <sstream>
#include <algorithm>	// for std::random_shuffle()
#include <deque>		// std::deque
#include <cstring>		// std::memcpy
#include <chrono>         // std::chrono::seconds
#include <thread>


#define CLASS_NAME	"RDMAClient"

template <typename T>
bool TPCW::TPCW_Client::isAddressInRange(uintptr_t lookupAddress, MemoryHandler<T> remoteMH) {
	if ((lookupAddress < (uintptr_t) remoteMH.rdmaHandler_.addr) || (lookupAddress >= (uintptr_t)remoteMH.rdmaHandler_.addr + remoteMH.regionSizeInBytes_)){
		PRINT_CERR(CLASS_NAME, __func__, "Accessing outside the region: " << lookupAddress << " NOT IN ["
				<< (uintptr_t) remoteMH.rdmaHandler_.addr << ", " << (uintptr_t)remoteMH.rdmaHandler_.addr + remoteMH.regionSizeInBytes_ << ") range");
		return false;
	}
	else return true;
}

void TPCW::TPCW_Client::fill_shopping_cart() {
	int item_id;
	int quantity;

	// First, choose K = config::ORDERLINE_PER_ORDER servers randomly.
	// We do so by shuffling the ordered list of servers, and choose the first K servers.
	int servers[config::SERVER_CNT];
	int selected_servers[config::tpcw_settings::ORDERLINE_PER_ORDER];

	for(int i = 0; i < config::SERVER_CNT; i++)
		servers[i] = i;
	std::random_shuffle(servers, servers + config::SERVER_CNT);

	for(int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++)
		selected_servers[i] = servers[i];


	DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 0: Cart contents: (Item ID,  Quantity, Server Num)");
	for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
		int server_num = selected_servers[i];
		item_id		= (server_num * config::tpcw_settings::ITEM_PER_SERVER) + (rand() % config::tpcw_settings::ITEM_PER_SERVER);	// generating in the range 0 to ITEM_CNT
		// item_id		= (i * ITEM_PER_SERVER) + zipf_get_sample(SKEWNESS_IN_ITEM_ACCESS, ITEM_PER_SERVER) - 1;	// generating in the range 0 to ITEM_CNT
		quantity	= (rand() % 5) + 1;			// the quantity of the item (not important)
		cart_.cart_lines[i].SCL_I_ID	= item_id;
		cart_.cart_lines[i].SCL_QTY	= quantity;
		cart_.cart_lines[i].SERVER_NUM = server_num;
		DEBUG_COUT (CLASS_NAME, __func__, "[Info] ... " <<  item_id << '\t' << quantity << '\t' << server_num);

		ds_ctx_[server_num].associated_cart_line = &cart_.cart_lines[i];
	}
}

int TPCW::TPCW_Client::acquire_read_ts() {
	MemoryHandler<primitive::timestamp_t> &remoteMH = oracleContext_->getRemoteMemoryKeys()->getRegion()->lastCommittedVector;

	primitive::timestamp_t *lookupAddress = (primitive::timestamp_t*)remoteMH.rdmaHandler_.addr;
	uint32_t size = (uint32_t)(remoteMH.regionSize_ * sizeof(primitive::timestamp_t));

	assert(isAddressInRange<primitive::timestamp_t>((uintptr_t)lookupAddress, remoteMH) == true);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
			oracleContext_->getQP(),
			localTimestampVector_->getRDMAHandler(),
			(uintptr_t)localTimestampVector_->getRegion(),
			&remoteMH.rdmaHandler_,
			(uintptr_t)lookupAddress,
			size,
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));
	return 0;
}

int TPCW::TPCW_Client::get_head_version(const size_t server_num) {
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;

	MemoryHandler<TPCW::ItemVersion> &remoteMH = dsCtx_[server_num]->getRemoteMemoryKeys()->getRegion()->itemTableHeadVersions;

	uint32_t size = (uint32_t) sizeof(TPCW::ItemVersion);

	if (dsCtx_[server_num]->getInstanceNum() != instanceNum_)
		std::memcpy( localMemory_->getItemHead()->getRegion(), &remoteMH.region_[item_id], size);

	else {
		size_t tableOffset = (size_t)(item_id * sizeof(TPCW::ItemVersion));		// offset of ItemVersion in ItemTable
		TPCW::ItemVersion *lookupAddress =  (TPCW::ItemVersion *)(tableOffset + ((uint64_t)remoteMH.rdmaHandler_.addr));

		assert(isAddressInRange<TPCW::ItemVersion>((uintptr_t)lookupAddress, remoteMH) == true);

		TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				dsCtx_[server_num]->getQP(),
				localMemory_->getItemHead()->getRDMAHandler(),
				localMemory_->getItemHead()->getRegion()[olNumber]
				&remoteMH.rdmaHandler_,
				(uintptr_t)lookupAddress,
				size,
				true));
	}

	return 0;
}

int TPCW::TPCW_Client::get_versions_pointers(const size_t server_num) {
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	if (ds_ctx_[server_num].instance_num == this->instance_num_) {
		std::memcpy(
				ds_ctx_[server_num].items_pointer,
				ds_ctx_[server_num].peer_memory_address_items_pointer_list + item_id * config::tpcw_settings::MAX_ITEM_VERSIONS,
				config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(Timestamp));
	}
	else {
		// Find the lookup address on the server remote memory
		size_t item_offset = (size_t)(item_id * config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(Timestamp));
		Timestamp *pointer_list_lookup_addr =  (Timestamp *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items_pointer_list.addr));

		// RDMA READ it from the server
		TEST_NZ( RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_READ,
				ds_ctx_[server_num].qp,
				ds_ctx_[server_num].mr_items_pointer,
				(uint64_t)ds_ctx_[server_num].items_pointer,
				&(ds_ctx_[server_num].peer_mr_items_pointer_list),
				(uint64_t)pointer_list_lookup_addr,
				(uint32_t)(config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(Timestamp)),
				false));
	}
	return 0;
}

int TPCW::TPCW_Client::acquire_commit_ts() {
	ts_ctx_.commit_timestamp = (primitive::timestamp_t)(next_epoch_ * client_cnt_ + client_id_);
	next_epoch_++;
	return 0;
}

int TPCW::TPCW_Client::acquire_item_lock(const size_t server_num, Timestamp &expected_lock, Timestamp &new_lock) {
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;

	//std::cout << "expected: " << expected_lock << "(" << expected_lock.toUUL() << ")" << std::endl;
	//std::cout << "new lock: " << new_lock << "(" << new_lock.toUUL() << ")" << std::endl;

	// and the actual content of the lock (before being swapped) will be stored in the following variable
	ds_ctx_[server_num].lock_item_region = ds_ctx_[server_num].items_head->write_timestamp.toUUL();

	size_t item_offset = (size_t)(item_id * sizeof(ItemVersion));
	uint64_t *lock_lookup_addr	= (uint64_t *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items_head.addr));

	TEST_NZ (RDMACommon::post_RDMA_CMP_SWAP(ds_ctx_[server_num].qp,
			ds_ctx_[server_num].mr_lock_item,
			(uint64_t)&ds_ctx_[server_num].lock_item_region,
			&(ds_ctx_[server_num].peer_mr_items_head),
			(uint64_t)lock_lookup_addr,
			(uint32_t)sizeof(uint64_t),
			expected_lock.toUUL(),
			new_lock.toUUL()));

	return 0;
}

int TPCW::TPCW_Client::revert_lock(const size_t server_num, Timestamp &new_lock) {
	int item_id		= ds_ctx_[server_num].associated_cart_line->SCL_I_ID;

	if (ds_ctx_[server_num].instance_num == this->instance_num_) {
		ds_ctx_[server_num].peer_memory_address_items_head[item_id].write_timestamp.copy(new_lock);
	}
	else {
		ds_ctx_[server_num].lock_item_region = new_lock.toUUL();

		size_t item_offset = (size_t)(item_id * sizeof(ItemVersion));
		uint64_t *lock_lookup_addr	= (uint64_t *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items_head.addr));

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ds_ctx_[server_num].qp,
				ds_ctx_[server_num].mr_lock_item,
				(uint64_t)&ds_ctx_[server_num].lock_item_region,
				&(ds_ctx_[server_num].peer_mr_items_head),
				(uint64_t)lock_lookup_addr,
				(uint32_t)sizeof(uint64_t),
				true));
	}
	return 0;
}

int TPCW::TPCW_Client::append_pointer_to_pointer_list(const size_t server_num) {
	// first, shift the pointers one to the right (this effectively drops the last element)
	for (int i = config::tpcw_settings::MAX_ITEM_VERSIONS - 2; i >= 0; i--)
		ds_ctx_[server_num].items_pointer[i+1] = ds_ctx_[server_num].items_pointer[i];

	// second, set the head of the pointer list to point to the head of the old versions
	ds_ctx_[server_num].items_pointer[0] = ds_ctx_[server_num].items_head->write_timestamp;

	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;

	if (ds_ctx_[server_num].instance_num == this->instance_num_) {
		std::memcpy(
				ds_ctx_[server_num].peer_memory_address_items_pointer_list + item_id * config::tpcw_settings::MAX_ITEM_VERSIONS,
				ds_ctx_[server_num].items_pointer,
				config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(Timestamp));

	}
	else {
		// write changes to the remote server
		size_t item_offset = (size_t)(item_id * config::MAX_ITEM_VERSIONS * sizeof(Timestamp));
		Timestamp *pointer_list_lookup_addr =  (Timestamp *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items_pointer_list.addr));

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ds_ctx_[server_num].qp,
				ds_ctx_[server_num].mr_items_pointer,
				(uint64_t)ds_ctx_[server_num].items_pointer,
				&(ds_ctx_[server_num].peer_mr_items_pointer_list),
				(uint64_t)pointer_list_lookup_addr,
				(uint32_t)(config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(Timestamp)),
				false));
	}

	return 0;
}

int TPCW::TPCW_Client::append_version_to_versions(const size_t server_num) {
	// find where should the head pointer be written to.
	primitive::version_offset_t version_offset = ds_ctx_[server_num].items_head->write_timestamp.getVersionOffset();

	// write the corresponding version to the version list
	int item_id = ds_ctx_[server_num].associated_cart_line->SCL_I_ID;

	if (ds_ctx_[server_num].instance_num == this->instance_num_) {
		std::memcpy(
				ds_ctx_[server_num].peer_memory_address_items_older_versions + item_id * config::tpcw_settings::MAX_ITEM_VERSIONS + version_offset,
				ds_ctx_[server_num].items_head,
				sizeof(ItemVersion));
	}
	else {
		// copy the version to the local RDMA buffer
		memcpy(ds_ctx_[server_num].items_older_version, ds_ctx_[server_num].items_head, sizeof(ItemVersion));

		size_t item_offset = (size_t)(item_id * config::tpcw_settings::MAX_ITEM_VERSIONS * sizeof(ItemVersion));
		ItemVersion *version_lookup_addr =
				(ItemVersion *)(
						item_offset +
						(uint64_t) version_offset * sizeof(ItemVersion) +
						((uint64_t)ds_ctx_[server_num].peer_mr_items_older_versions.addr)
				);

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ds_ctx_[server_num].qp,
				ds_ctx_[server_num].mr_items_older_version,
				(uint64_t)ds_ctx_[server_num].items_older_version,
				&(ds_ctx_[server_num].peer_mr_items_older_versions),
				(uint64_t)version_lookup_addr ,
				(uint32_t)sizeof(ItemVersion),
				false));

	}
	return 0;
}


int TPCW::TPCW_Client::install_and_unlock(const size_t server_num) {
	int item_id		= ds_ctx_[server_num].associated_cart_line->SCL_I_ID;
	int quantity	= ds_ctx_[server_num].associated_cart_line->SCL_QTY;

	primitive::lock_status_t	lock_status = 0;
	primitive::version_offset_t	versionOffset = (primitive::version_offset_t)(ds_ctx_[server_num].items_head->write_timestamp.getVersionOffset() + 1) % config::tpcw_settings::MAX_ITEM_VERSIONS;

	ds_ctx_[server_num].items_head[0].write_timestamp.setAll(lock_status, versionOffset, client_id_, ts_ctx_.commit_timestamp);
	ds_ctx_[server_num].items_head[0].item.I_STOCK -= quantity;
	//std::cout << "what is written: " << ds_ctx_[server_num].items_head[0].write_timestamp << std::endl;

	if (ds_ctx_[server_num].items_head[0].item.I_STOCK < 10)
		ds_ctx_[server_num].items_head[0].item.I_STOCK += 20;

	if (dsCtx_[server_num]->getInstanceNum() != instanceNum_)
		std::memcpy(
				ds_ctx_[server_num].peer_memory_address_items_head + item_id,
				ds_ctx_[server_num].items_head,
				sizeof(ItemVersion));
	}
	else {
		// Find the lookup address on the server remote memory
		size_t item_offset = (item_id * sizeof(ItemVersion));
		ItemVersion *item_lookup_addr =  (ItemVersion *)(item_offset + ((uint64_t)ds_ctx_[server_num].peer_mr_items_head.addr));

		TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
				ds_ctx_[server_num].qp,
				ds_ctx_[server_num].mr_items_head,
				(uint64_t)ds_ctx_[server_num].items_head,
				&(ds_ctx_[server_num].peer_mr_items_head),
				(uint64_t)item_lookup_addr,
				(uint32_t)sizeof(ItemVersion),
				true));
	}

	DEBUG_COUT (CLASS_NAME, __func__, "[WRIT] ... (Client " << client_id_ << ") Successfully install and unlock a new version for item " << item_id
			<< " ("<< ds_ctx_[server_num].items_head[0].write_timestamp <<")");

	return 0;
}

int TPCW::TPCW_Client::submit_trx_result() {

	size_t offset = client_id_ * sizeof(primitive::timestamp_t);
	primitive::timestamp_t *write_addr	= (primitive::timestamp_t *)(offset + (uint64_t)ts_ctx_.peer_mr_last_trx_timestamp_per_client.addr);

	TEST_NZ (RDMACommon::post_RDMA_READ_WRT(IBV_WR_RDMA_WRITE,
			ts_ctx_.qp,
			ts_ctx_.mr_commit_timestamp,
			(uint64_t)&ts_ctx_.commit_timestamp,
			&(ts_ctx_.peer_mr_last_trx_timestamp_per_client),
			(uint64_t)write_addr,
			(uint32_t)sizeof(primitive::timestamp_t),
			true));

	TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));


	DEBUG_COUT (CLASS_NAME, __func__, "[Sent] Client " << client_id_ << " sent trx result for CTS " << ts_ctx_.commit_timestamp << " to the TS server");
	return 0;
}

inline
std::string TPCW::TPCW_Client::pointer_to_string(const size_t server_num) const{
	std::ostringstream stream;
	for (size_t i = 0; i < config::tpcw_settings::MAX_ITEM_VERSIONS; i++)
		stream << ds_ctx_[server_num].items_pointer[i] << ", ";
	return stream.str();
}

inline
std::string TPCW::TPCW_Client::read_snapshot_to_string() const{
	std::ostringstream stream;
	for (size_t c = 0; c < client_cnt_; c++)
		stream << "[" << c << "]=" << ts_ctx_.last_trx_timestamp_per_client[c]  << ", ";
	return stream.str();
}

primitive::client_id_t TPCW::TPCW_Client::find_commiting_client(Timestamp &versionTimestamp) {
	return versionTimestamp.getClientID();
}

int TPCW::TPCW_Client::startTransactions() {
	int		abort_cnt = 0;
	int 	abort_due_to_inconsistent_snapshot_cnt = 0, abort_due_to_lock_cnt = 0;
	std::deque<Result> recent_commit_results;
	int recent_abort_cnt = 0;
	int snapshot_acquisition_cnt = 0;

	struct timespec firstRequestTime, lastRequestTime;				// for calculating TPMS
	struct timespec before_read_ts, after_read_ts, after_fetch_info, after_commit_ts, after_lock, after_unlock, after_result_submission;
	double avg_read_ts = 0, avg_fetch_info = 0, avg_commit_ts = 0, avg_lock = 0, avg_unlock = 0, avg_result_submission = 0;

	Timestamp	expected_lock[config::SERVER_CNT], new_lock[config::SERVER_CNT];

	clock_gettime(CLOCK_REALTIME, &firstRequestTime);	// Fire the  timer
	for (int trx_num = 1; trx_num <= config::tpcw_settings::TRANSACTION_CNT; trx_num++){
		DEBUG_COUT (CLASS_NAME, __func__, std::endl << "[Info] Client " << client_id_ << " starts transaction #" << trx_num);

		// ************************************************
		//	Constructing the shopping cart
		// ************************************************
		fill_shopping_cart();


		// ************************************************
		//	Acquire read timestamp
		// ************************************************
		clock_gettime(CLOCK_REALTIME, &before_read_ts);
		if (config::ADAPTIVE_ABORT_RATE == true) {
			// Adaptive snapshot acquisition is active. The client gets a new snapshot only if the current abort rate is too high

			//int	recent_abort_cnt = 0;
			if (recent_commit_results.size() > 0){
				// compute the average recent abort rate
				//				for (unsigned i=0; i<recent_commit_results.size(); i++){
				//					if (recent_commit_results.at(i) == Result::ABORTED)
				//						recent_abort_cnt++;
				//				}
				double abort_rate_recent_average = (double)recent_abort_cnt / recent_commit_results.size();
				if (abort_rate_recent_average >= config::MAX_ABORT_RATE) {
					// abort rate is too high. get a new snapshot
					snapshot_acquisition_cnt++;
					TEST_NZ (acquire_read_ts());
					DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 1: Client " << client_id_ << " received snapshot from oracle (" << read_snapshot_to_string() << ")");
				}
				else
					// re-use the old snapshot
					DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 1: Client " << client_id_ << " re-used snapshot (" << read_snapshot_to_string() << ")");
			}
			else{
				// it's first time. client must get the snapshot anyways
				snapshot_acquisition_cnt++;
				TEST_NZ (acquire_read_ts());
				DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 1: Client " << client_id_ << " received snapshot from oracle (" << read_snapshot_to_string() << ")");
			}
		}
		else {
			// it's not adaptive. so client gets the snapshot every time.
			snapshot_acquisition_cnt++;
			TEST_NZ (acquire_read_ts());
			DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 1: Client " << client_id_ << " received snapshot from oracle (" << read_snapshot_to_string() << ")");
		}
		clock_gettime(CLOCK_REALTIME, &after_read_ts);

		// ************************************************
		//	Read records in read-set + Read Pointers
		// ************************************************
		// TODO: Should be fixed. multiple requests to the same server should be sent with only one signalled request
		for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
			size_t server_num = cart_.cart_lines[i].SERVER_NUM; //size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
			TEST_NZ(get_versions_pointers(server_num));
			TEST_NZ(get_head_version(server_num));
		}

		for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
			size_t server_num = cart_.cart_lines[i].SERVER_NUM;
			if (dsCtx_[server_num]->getInstanceNum() != instanceNum_)
				TEST_NZ (RDMACommon::poll_completion(context_->getSendCq()));

			DEBUG_COUT (CLASS_NAME, __func__, "[Read] Step 2-" << i << ": Client " << client_id_ << " received information for item " << ds_ctx_[server_num].items_head->item.I_ID
					<< "(ts: " << ds_ctx_[server_num].items_head->write_timestamp << ") from " << ds_ctx_[server_num].server_address);

			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 2-" << i << ": Client " << client_id_ << " received the following pointer list for item "
					<< ds_ctx_[server_num].items_head->item.I_ID << ": " << pointer_to_string(server_num));
		}

		DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 2: Client " << client_id_ << " received information for all items");
		clock_gettime(CLOCK_REALTIME, &after_fetch_info);


		// ************************************************
		//	Acquire Commit timestamp
		// ************************************************
		acquire_commit_ts();
		DEBUG_COUT (CLASS_NAME, __func__, "[LOCL] Step 3: Client " << client_id_ << " acquired commit timestamp " << ts_ctx_.commit_timestamp);
		clock_gettime(CLOCK_REALTIME, &after_commit_ts);


		// ************************************************
		//	Check whether fetched items are from a consistent snapshot, and not locked
		// ************************************************
		// IMPORTANT: This step has to be done AFTER acquiring CTS. It follows that if the snapshot is already overwritten, the transaction has to ABORT and
		// flip the bit on the TS server.
		// It is tempting to think that this step can be done BEFORE acquiring the commit timestamp, which would subsequently allow the client to silently abort
		// (by avoid flipping the bit on TS server if the transaction has to abort). However, this will result in a potential unlimited loop, where the client reads the same snapshot over and over
		// again, and each time aborts because the snapshot is no longer valid. However, the sweeper cannot make progress because this client aborts silently.
		bool abort_flag = false;
		for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
			size_t server_num = cart_.cart_lines[i].SERVER_NUM; //size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
			if (ds_ctx_[server_num].items_head->write_timestamp.getLockStatus() != 0) {
				// item is already locked
				abort_flag = true;
				DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 4: Client " << client_id_ << " locked version for item " << ds_ctx_[server_num].items_head->item.I_ID);
			}
			else {
				primitive::client_id_t committed_client = find_commiting_client(ds_ctx_[server_num].items_head->write_timestamp);
				if (committed_client == client_id_)
					// regardless of whether the version matches the snapshot or not, the version is installed by the client itself, so is valid
					// when is it useful? when using adaptive abort rate control.
					continue;

				if (ds_ctx_[server_num].items_head->write_timestamp.getTimestamp() > ts_ctx_.last_trx_timestamp_per_client[committed_client]) {
					// from a later snapshot, so not useful
					abort_flag = true;
					DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 4: (Client " << client_id_ << ") Inconsistent version for item "
							<< ds_ctx_[server_num].items_head->item.I_ID << " (Expected: " << ts_ctx_.last_trx_timestamp_per_client[committed_client] << " or lower."
							<< " Received: " << ds_ctx_[server_num].items_head->write_timestamp.getTimestamp() << ")" );
				}
			}
		}
		if (abort_flag == true) {
			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 4: (Client " << client_id_ << ") NOT all received versions are consistent with READ snapshot or some are locked --> ** ABORT **");
			abort_cnt++;
			abort_due_to_inconsistent_snapshot_cnt++;
			recent_abort_cnt++;
			submit_trx_result();

			recent_commit_results.push_back(Result::ABORTED);
			if (recent_commit_results.size() > config::ADAPTIVE_WINDOW_SIZE) {
				if (recent_commit_results.front() == Result::ABORTED)
					recent_abort_cnt--;
				recent_commit_results.pop_front();
			}

			continue;
		}
		else DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 4: (Client " << client_id_ << ") All received versions are consistent with READ snapshot, and all are unlocked");


		size_t found_version_index = 0;

		// ************************************************
		// Acquire locks for items in write-set
		// ************************************************
		for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
			size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;

			// lock expected before locking
			primitive::lock_status_t 	lock_status		= 0;
			primitive::version_offset_t	version_offset	= ds_ctx_[server_num].items_head[found_version_index].write_timestamp.getVersionOffset();
			primitive::client_id_t		clientID		= ds_ctx_[server_num].items_head[found_version_index].write_timestamp.getClientID();
			primitive::timestamp_t		timestamp		= ds_ctx_[server_num].items_head[found_version_index].write_timestamp.getTimestamp();
			expected_lock[server_num].setAll(lock_status, version_offset, clientID, timestamp);

			// new lock
			lock_status	= 1;
			version_offset		= (primitive::version_offset_t)(ds_ctx_[server_num].items_head->write_timestamp.getVersionOffset() + 1) % config::tpcw_settings::MAX_ITEM_VERSIONS;
			clientID	= this->client_id_;
			timestamp	= ts_ctx_.commit_timestamp;
			new_lock[server_num].setAll(lock_status, version_offset, clientID, timestamp);
			acquire_item_lock(server_num, expected_lock[server_num], new_lock[server_num]);
		}

		bool	successful_locking_servers[config::SERVER_CNT] = { false };		// initializes the entire array to false
		abort_flag = false;
		for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
			size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
			TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
			//std::cout << "before swapping: " << ds_ctx_[server_num].lock_item_region << std::endl;

			// Byte swapping, since values read by atomic operations are in Big Endian order
			ds_ctx_[server_num].lock_item_region = bigEndianToHost(ds_ctx_[server_num].lock_item_region);

			Timestamp received_ts(ds_ctx_[server_num].lock_item_region);

			// Check if CASing the lock was successful
			if (expected_lock[server_num].isEqual(received_ts)) {
				DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Step 5-" << i << ": (Client " << client_id_ << ") Lock on item " << cart_.cart_lines[i].SCL_I_ID << " successfully acquired.");
				successful_locking_servers[server_num] = true;
			}
			else {
				DEBUG_COUT (CLASS_NAME, __func__, "[CMSW] Step 5-" << i << ": (Client " << client_id_ << ") Failed to lock item " << cart_.cart_lines[i].SCL_I_ID);
				DEBUG_COUT (CLASS_NAME, __func__, "........ Expected lock: " << expected_lock[server_num]);
				DEBUG_COUT (CLASS_NAME, __func__, "........ Existing lock: " << received_ts);

				successful_locking_servers[server_num] = false;
				abort_flag = true;
			}
		}

		// Check if all locks are successfully acquired
		if (abort_flag == true) {
			for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
				size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				if (successful_locking_servers[server_num] == true)
					TEST_NZ(revert_lock(server_num, expected_lock[server_num]));
			}
			for (int i = 0; i < config::ORDERLINE_PER_ORDER; i++) {
				size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				if (successful_locking_servers[server_num] == true) {
					if (ds_ctx_[server_num].instance_num != this->instance_num_)
						TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
					DEBUG_COUT (CLASS_NAME, __func__, "[Writ] Step 5: (Client " << client_id_ << ") Lock reverted on item " << ds_ctx_[server_num].items_head->item.I_ID);
				}
			}
			DEBUG_CERR (CLASS_NAME, __func__, "[Info] Step 5: (Client " << client_id_ << ") Lock on all items could not be acquired --> ** ABORT **");
			abort_cnt++;
			abort_due_to_lock_cnt++;
			// TODO: do we actually need to inform the TS server about the aborts?? probably no.
			submit_trx_result();

			recent_commit_results.push_back(Result::ABORTED);
			recent_abort_cnt++;
			if (recent_commit_results.size() > config::ADAPTIVE_WINDOW_SIZE){
				if (recent_commit_results.front() == Result::ABORTED)
					recent_abort_cnt--;
				recent_commit_results.pop_front();
			}
			continue;
		}
		else {
			clock_gettime(CLOCK_REALTIME, &after_lock);
			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 5: (Client " << client_id_ << ") All locks successfully acquired");


			// ************************************************
			//	Append old version to the versions list, and update the pointers list
			// ************************************************
			for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
				size_t server_num = cart_.cart_lines[i].SERVER_NUM; // size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				append_pointer_to_pointer_list(server_num);
				append_version_to_versions(server_num);
			}
			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 6 : Client " << client_id_ << " added the pointers and pointers");


			// ************************************************
			//	Install and unlock the new versions
			// ************************************************
			for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
				size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				install_and_unlock(server_num);
			}
			for (int i = 0; i < config::tpcw_settings::ORDERLINE_PER_ORDER; i++) {
				size_t server_num = cart_.cart_lines[i].SERVER_NUM;	// size_t server_num = cart_.cart_lines[i].SCL_I_ID / config::ITEM_PER_SERVER;
				if (ds_ctx_[server_num].instance_num != this->instance_num_)
					TEST_NZ (RDMACommon::poll_completion(ds_ctx_[server_num].send_cq));
			}
			DEBUG_COUT (CLASS_NAME, __func__, "[Info] Step 7: Client " << client_id_ << " installed and unlocked the new versions");
			clock_gettime(CLOCK_REALTIME, &after_unlock);


			// ************************************************
			//	Submit the result to the timestamp server
			// ************************************************
			submit_trx_result();
			recent_commit_results.push_back(Result::COMMITTED);
			if (recent_commit_results.size() > config::ADAPTIVE_WINDOW_SIZE){
				if (recent_commit_results.front() == Result::ABORTED)
					recent_abort_cnt--;
				recent_commit_results.pop_front();
			}

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

			t = (double)( after_unlock.tv_sec - after_lock.tv_sec ) * 1E9 + (double)( after_unlock.tv_nsec - after_lock.tv_nsec );
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
	clock_gettime(CLOCK_REALTIME, &lastRequestTime);	// Fire the  timer

	int committed_cnt = config::tpcw_settings::TRANSACTION_CNT - abort_cnt;
	double micro_elapsed_time = ( (double)( lastRequestTime.tv_sec - firstRequestTime.tv_sec ) * 1E9 + (double)( lastRequestTime.tv_nsec - firstRequestTime.tv_nsec ) ) / 1000;
	double T_P_MILISEC = (double)(committed_cnt / (double)(micro_elapsed_time / 1000));
	double success_rate = (double)committed_cnt / config::tpcw_settings::TRANSACTION_CNT;

	avg_read_ts /= 1000;
	avg_fetch_info /= 1000;
	avg_commit_ts /= 1000;
	avg_lock /= 1000;
	avg_unlock /= 1000;
	avg_result_submission /= 1000;

	std::cout << "[Stat] Avg read ts (u sec):	" << (double)avg_read_ts / committed_cnt << std::endl;
	std::cout << "[Stat] Avg fetch (u sec): 	" << (double)avg_fetch_info / committed_cnt << std::endl;
	std::cout << "[Stat] Avg commit ts (u sec):	" << (double)avg_commit_ts / committed_cnt << std::endl;
	std::cout << "[Stat] Avg lock (u sec):  	" << (double)avg_lock / committed_cnt << std::endl;
	std::cout << "[Stat] Avg install & unlock (u sec):	" << (double)avg_unlock / committed_cnt << std::endl;
	std::cout << "[Stat] Avg res subm (u sec):	" << (double)avg_result_submission / committed_cnt << std::endl;
	std::cout << "[Stat] Avg cumulative(u sec):	" << (double)micro_elapsed_time / config::tpcw_settings::TRANSACTION_CNT << std::endl;
	std::cout << "[Stat] " << committed_cnt << " committed, " << abort_cnt << " aborted. success rate:	" << success_rate << std::endl;
	std::cout << "[Stat] Avg abort type I (snapshot) ratio	" << (double)abort_due_to_inconsistent_snapshot_cnt/abort_cnt << std::endl;
	std::cout << "[Stat] Avg abort type II (lock) ratio	" << (double)abort_due_to_lock_cnt/abort_cnt << std::endl;
	std::cout << "[Stat] Snapshot acquisition ratio	" << (double)snapshot_acquisition_cnt/config::tpcw_settings::TRANSACTION_CNT << std::endl;
	std::cout << "[Stat] Committed Trx per millisec:	" <<  T_P_MILISEC << std::endl;
	return 0;
}

TPCW::TPCW_Client::TPCW_Client(unsigned instanceNum, uint8_t ibPort)
: instanceNum_(instanceNum),
  ibPort_(ibPort){
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed
	//zipf_initialize(config::SKEWNESS_IN_ITEM_ACCESS, config::ITEM_PER_SERVER);
	nextEpoch_ = (primitive::timestamp_t) 1ULL;

	context_ 		= new RDMAContext(ibPort_);
	localMemory_ 	= new TPCW::BuyConfirmLocalMemory(*context_);
	//sessionState_ 	= new SessionState(homeWarehouseID, (primitive::timestamp_t) 1ULL);
	zipf_initialize(config::tpcw_settings::SKEWNESS_IN_ITEM_ACCESS, config::tpcw_settings::ITEM_PER_SERVER);

	// ************************************************
	//	Connect to Oracle
	// ************************************************
	int sockfd;
	TEST_NZ (utils::establish_tcp_connection(config::TIMESTAMP_SERVER_ADDR, config::TIMESTAMP_SERVER_PORT, &sockfd));
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to Oracle");
	oracleContext_ = new OracleContext(sockfd, config::TIMESTAMP_SERVER_PORT, config::TIMESTAMP_SERVER_IB_PORT, *context_);

	// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
	RDMACommon::post_RECEIVE(
			oracleContext_->getQP(),
			oracleContext_->getRemoteMemoryKeys()->getRDMAHandler(),
			(uintptr_t)oracleContext_->getRemoteMemoryKeys()->getRegion(),
			(uint32_t)oracleContext_->getRemoteMemoryKeys()->getRegionSizeInByte());


	oracleContext_->activateQueuePair(*context_);
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to Oracle");

	TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
	DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from Oracle");

	// Set client ID and client cnt
	clientID_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_id;
	clientCnt_ = oracleContext_->getRemoteMemoryKeys()->getRegion()->client_cnt;
	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is assigned ID = " << clientID_ << " out of " << clientCnt_ << " clients");

	localTimestampVector_	= new RDMARegion<primitive::timestamp_t>(clientCnt_, *context_, IBV_ACCESS_LOCAL_WRITE);


	// ************************************************
	//	Connect to Servers
	// ************************************************
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::establish_tcp_connection(config::SERVER_ADDR[i], config::TCP_PORT[i], &sockfd));

		// build server context
		dsCtx_.push_back(new TPCW_ServerContext(sockfd, config::SERVER_ADDR[i], config::TCP_PORT[i], config::IB_PORT[i], instanceNum, *context_));

		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Connection established to server " << i);


		// before connecting the queue pairs, we post the RECEIVE job to be ready for the server's message containing its memory locations
		RDMACommon::post_RECEIVE(
				dsCtx_[i]->getQP(),
				dsCtx_[i]->getRemoteMemoryKeys()->getRDMAHandler(),
				(uintptr_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegion(),
				(uint32_t)dsCtx_[i]->getRemoteMemoryKeys()->getRegionSizeInByte());

		dsCtx_[i]->activateQueuePair(*context_);
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] QPed to server " << i);

		TEST_NZ(RDMACommon::poll_completion(context_->getRecvCq()));
		DEBUG_COUT(CLASS_NAME, __func__, "[Recv] buffers info from server " << i);
	}

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Starting transactions ");

	startTransactions();

	DEBUG_COUT(CLASS_NAME, __func__, "[Info] Client is done, and is ready to destroy its resources!");
	for (int i = 0; i < config::SERVER_CNT; i++){
		TEST_NZ (utils::sock_sync (dsCtx_[i]->getSockFd()));	// just send a dummy char back and forth
		DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified server " << i << " it's done");
		delete dsCtx_[i];
	}

	TEST_NZ (utils::sock_sync (oracleContext_->getSockFd()));	// just send a dummy char back and forth
	DEBUG_COUT(CLASS_NAME, __func__, "[Conn] Notified Oracle it's done");
}
