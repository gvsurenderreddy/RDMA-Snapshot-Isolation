/*
 *	ServerContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef SERVER_CONTEXT_H_
#define SERVER_CONTEXT_H_

#include "../../../rdma-region/RDMARegion.hpp"
#include "../server/ServerMemoryKeys.hpp"
#include "../tables/CustomerTable.hpp"
#include "../tables/DistrictTable.hpp"
#include "../tables/HistoryTable.hpp"
#include "../tables/ItemTable.hpp"
#include "../tables/NewOrderTable.hpp"
#include "../tables/OrderLineTable.hpp"
#include "../tables/OrderTable.hpp"
#include "../tables/StockTable.hpp"
#include "../tables/WarehouseTable.hpp"
#include "../../../basic-types/timestamp.hpp"
#include <string>	// for std::string
#include <infiniband/verbs.h>	// for ibv_qp
#include <cstdint>	// uintX_t


class ServerContext {
private:
	int 		sockfd_;
	std::string	serverAddress_;
	uint16_t	tcpPort_;
	uint8_t 	ibPort_;
	unsigned	instanceNum_;
	struct	ibv_qp	*qp_;

	// ShoppingCartLine	*associated_cart_line;

	// RDMA region for storing remote memory keys
	RDMARegion<ServerMemoryKeys> *peerMemoryKeys_;

	// RDMA region for local buffers
	RDMARegion<TPCC::CustomerVersion>	*localCustomerHead_;
	RDMARegion<Timestamp> 				*localCustomerTS_;
	RDMARegion<TPCC::CustomerVersion>	*localCustomerOlderVersions_;

	RDMARegion<TPCC::DistrictVersion>	*localDistrictHead_;
	RDMARegion<Timestamp> 				*localDistrictTS_;
	RDMARegion<TPCC::DistrictVersion>	*localDistrictOlderVersions_;

	RDMARegion<TPCC::ItemVersion>	*localItemHead_;
	RDMARegion<Timestamp> 			*localItemTS_;
	RDMARegion<TPCC::ItemVersion>	*localItemOlderVersions_;

	RDMARegion<TPCC::NewOrderVersion>	*localNewOrderHead_;
	RDMARegion<Timestamp> 				*localNewOrderTS_;
	RDMARegion<TPCC::NewOrderVersion>	*localNewOrderOlderVersions_;

	RDMARegion<TPCC::OrderLineVersion>	*localOrderLineHead_;
	RDMARegion<Timestamp> 				*localOrderLineTS_;
	RDMARegion<TPCC::OrderLineVersion>	*localOrderLineOlderVersions_;

	RDMARegion<TPCC::OrderVersion>	*localOrderHead_;
	RDMARegion<Timestamp> 			*localOrderTS_;
	RDMARegion<TPCC::OrderVersion>	*localOrderOlderVersions_;

	RDMARegion<TPCC::StockVersion>	*localStockHead_;
	RDMARegion<Timestamp> 			*localStockTS_;
	RDMARegion<TPCC::StockVersion>	*localStockOlderVersions_;

	RDMARegion<TPCC::WarehouseVersion>	*localWarehouseHead_;
	RDMARegion<Timestamp> 				*localWarehouseTS_;
	RDMARegion<TPCC::WarehouseVersion>	*localWarehouseOlderVersions_;


	// uint64_t	lock_item_region;
	// struct ibv_mr *mr_lock_item;

public:
	ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context);
	~ServerContext();
	std::string getServerAddress() const;
	uint16_t getTcpPort() const;
	int getSockFd() const;
	ibv_qp* getQP() const;
	RDMARegion<ServerMemoryKeys>* getRemoteMemoryKeys();

	RDMARegion<TPCC::CustomerVersion>* getLocalCustomerHead();
	RDMARegion<Timestamp>* getLocalCustomerTS();
	RDMARegion<TPCC::CustomerVersion>* getLocalCustomerOlderVersions();

	RDMARegion<TPCC::DistrictVersion>* getLocalDistrictHead();
	RDMARegion<Timestamp>* getLocalDistrictTS();
	RDMARegion<TPCC::DistrictVersion>* getLocalDistrictOlderVersions();

	RDMARegion<TPCC::ItemVersion>* getLocalItemHead();
	RDMARegion<Timestamp>* getLocalItemTS();
	RDMARegion<TPCC::ItemVersion>* getLocalItemOlderVersions();

	RDMARegion<TPCC::NewOrderVersion>* getLocalNewOrderHead();
	RDMARegion<Timestamp>* getLocalNewOrderTS();
	RDMARegion<TPCC::NewOrderVersion>* getLocalNewOrderOlderVersions();

	RDMARegion<TPCC::OrderLineVersion>* getLocalOrderLineHead();
	RDMARegion<Timestamp>* getLocalOrderLineTS();
	RDMARegion<TPCC::OrderLineVersion>* getLocalOrderLineOlderVersions();

	RDMARegion<TPCC::OrderVersion>* getLocalOrderHead();
	RDMARegion<Timestamp>* getLocalOrderTS();
	RDMARegion<TPCC::OrderVersion>* getLocalOrderOlderVersions();

	RDMARegion<TPCC::StockVersion>* getLocalStockHead();
	RDMARegion<Timestamp>* getLocalStockTS();
	RDMARegion<TPCC::StockVersion>* getLocalStockOlderVersions();

	RDMARegion<TPCC::WarehouseVersion>* getLocalWarehouseHead();
	RDMARegion<Timestamp>* getLocalWarehouseTS();
	RDMARegion<TPCC::WarehouseVersion>* getLocalWarehouseOlderVersions();


	void activateQueuePair(RDMAContext &context);

	ServerContext& operator=(const ServerContext&) = delete;	// Disallow copying
	ServerContext(const ServerContext&) = delete;				// Disallow copying
};
#endif // SERVER_CONTEXT_H_
