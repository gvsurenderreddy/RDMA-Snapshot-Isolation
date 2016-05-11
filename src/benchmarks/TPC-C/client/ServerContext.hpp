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
#include "../index-messages/IndexRequestMessage.hpp"
#include "../index-messages/IndexResponseMessage.hpp"
#include "../index-messages/CustomerNameIndexRespMsg.hpp"
#include "../index-messages/LargestOrderForCustomerIndexRespMsg.hpp"
#include "../index-messages/Last20OrdersIndexResMsg.hpp"
#include "../index-messages/OldestUndeliveredOrderIndexResMsg.hpp"
#include "../tables/TPCCDB.hpp"
#include <string>	// for std::string
#include <infiniband/verbs.h>	// for ibv_qp
#include <cstdint>	// uintX_t

namespace TPCC{
class ServerContext {
private:
	std::ostream &os_;
	int 		sockfd_;
	std::string	serverAddress_;
	uint16_t	tcpPort_;
	uint8_t 	ibPort_;
	unsigned	instanceNum_;
	TPCC::TPCCDB	*db_;	// this object may __ONLY__ be used by the client only if it sits within the same address space of the server

	struct	ibv_qp	*qp_;

	// RDMA region for storing remote memory keys
	RDMARegion<ServerMemoryKeys> *peerMemoryKeys_;

	// RDMA region for index request and response
	RDMARegion<TPCC::IndexRequestMessage>		*indexRequestMessage_;
	RDMARegion<TPCC::IndexResponseMessage>		*indexResponseMessage_;
	RDMARegion<TPCC::CustomerNameIndexRespMsg>	*customerNameIndexRespMsg_;
	RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>	*largestOrderForCustomerIndexRespMsg_;
	RDMARegion<TPCC::Last20OrdersIndexResMsg>	*last20OrdersIndexRespMsg_;
	RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg>	*oldestUndeliveredOrderIndexRespMsg_;

public:
	ServerContext(std::ostream &os, const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, RDMAContext &context);
	~ServerContext();
	std::string getServerAddress() const;
	uint16_t getTcpPort() const;
	int getSockFd() const;
	unsigned getInstanceNum() const;
	TPCC::TPCCDB* getDatabaseObject();
	ibv_qp* getQP() const;
	RDMARegion<ServerMemoryKeys>* getRemoteMemoryKeys();
	RDMARegion<TPCC::IndexRequestMessage>* getIndexRequestMessage();
	RDMARegion<TPCC::IndexResponseMessage>* getRegisterOrderIndexResponseMessage();
	RDMARegion<TPCC::CustomerNameIndexRespMsg>* getCustomerNameIndexResponseMessage();
	RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>* getLargestOrderForCustomerIndexResponseMessage();
	RDMARegion<TPCC::Last20OrdersIndexResMsg>* getLast20OrdersIndexResponseMessage();
	RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg>* getOldestUndeliveredOrderIndexResponseMessage();
	RDMARegion<TPCC::IndexResponseMessage>* getRegisterDeliveryIndexResponseMessage();

	void setInstanceNum(unsigned instanceNum);
	void setDatabaseObject(TPCC::TPCCDB	*db);
	void activateQueuePair(RDMAContext &context);
	ServerContext& operator=(const ServerContext&) = delete;	// Disallow copying
	ServerContext(const ServerContext&) = delete;				// Disallow copying
};
}	// namespace TPCC
#endif // SERVER_CONTEXT_H_
