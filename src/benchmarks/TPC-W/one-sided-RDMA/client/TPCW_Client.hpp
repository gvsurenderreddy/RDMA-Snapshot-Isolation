/*
 *	RDMAClient.hpp
 *
 *	Created on: 25.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_H_
#define RDMA_CLIENT_H_

#include "TPCW_ServerContext.hpp"
#include "../../../../oracle/OracleContext.hpp"
#include "../../../../../config.hpp"
#include "../../../../util/RDMACommon.hpp"
#include "../../../../basic-types/PrimitiveTypes.hpp"
#include "../../../../rdma-region/RDMAContext.hpp"
#include "../../../../rdma-region/RDMARegion.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


typedef message::TransactionResult::Result Result;

namespace TPCW{
class TPCW_Client{
private:
	primitive::client_id_t	clientID_;
	const unsigned instanceNum_;
	const uint8_t ibPort_;
	size_t	clientCnt_;
	std::vector<TPCW_ServerContext*> dsCtx_;
	TPCW::BuyConfirmLocalMemory* localMemory_;
	RDMAContext *context_;
	OracleContext *oracleContext_;
//	SessionState *sessionState_;
	RDMARegion<primitive::timestamp_t> *localTimestampVector_;

	primitive::timestamp_t	nextEpoch_;

	struct Cart {
		ShoppingCartLine	cart_lines[config::tpcw_settings::ORDERLINE_PER_ORDER];
	} cart_;

	void	fill_shopping_cart();
	int		acquire_read_ts();
	int		get_head_version(const size_t server_num);
	int		get_versions_pointers(const size_t server_num);
	int 	acquire_commit_ts();
	int		acquire_item_lock(const size_t server_num, Timestamp &expectedTS, Timestamp &newTS);
	int		revert_lock(const size_t server_num, Timestamp &new_lock);
	int		append_pointer_to_pointer_list(const size_t server_num);
	int		append_version_to_versions(const size_t server_num);
	int		install_and_unlock(const size_t server_num);
	int		release_lock(const size_t server_num);
	int 	submit_trx_result();
	primitive::client_id_t	find_commiting_client(Timestamp &versionTimestamp);
	std::string pointer_to_string(const size_t server_num) const;
	std::string read_snapshot_to_string() const;
	int		startTransactions();

	template <typename T>
	bool isAddressInRange(uintptr_t lookupAddress, MemoryHandler<T> remoteMH);


public:
	TPCW_Client(unsigned instanceNum, uint8_t ibPort);

	TPCW_Client& operator=(const TPCW_Client&) = delete;	// Disallow copying
	TPCW_Client(const TPCW_Client&) = delete;				// Disallow copying
};
}	// namespace TPCW

#endif /* RDMA_CLIENT_H_ */
