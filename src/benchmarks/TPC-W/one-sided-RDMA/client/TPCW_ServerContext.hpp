/*
 *	RDMAClientContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CLIENT_CONTEXT_H_
#define RDMA_CLIENT_CONTEXT_H_

#include "../../../../../config.hpp"
#include "../../../../util/RDMACommon.hpp"
#include "../../../../rdma-region/RDMARegion.hpp"
#include "../../basic-types/timestamp.hpp"
#include "../../one-sided-RDMA/server/TPCW_ServerMemoryKeys.hpp"
#include "../../tables/cc_xacts_version.hpp"
#include "../../tables/item_version.hpp"
#include "../../tables/order_line_version.hpp"
#include "../../tables/orders_version.hpp"
#include "../../tables/shopping_cart_line.hpp"

namespace TPCW{
class TPCW_ServerContext{
private:
	int 		sockfd_;
	std::string	serverAddress_;
	uint16_t	tcpPort_;
	uint8_t 	ibPort_;
	unsigned	instanceNum_;

	//ShoppingCartLine	*associated_cart_line;
	unsigned instance_num;
	struct	ibv_qp	*qp_;

	// RDMA region for storing remote memory keys
	RDMARegion<TPCW::TPCW_ServerMemoryKeys> *peerMemoryKeys_;


public:
	
	TPCW_ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context);
	~TPCW_ServerContext();
	std::string getServerAddress() const;
	uint16_t getTcpPort() const;
	int getSockFd() const;
	unsigned getInstanceNum() const;
	ibv_qp* getQP() const;
	RDMARegion<TPCW::TPCW_ServerMemoryKeys>* getRemoteMemoryKeys();

	void activateQueuePair(RDMAContext &context);

	TPCW_ServerContext& operator=(const ServerContext&) = delete;	// Disallow copying
	TPCW_ServerContext(const ServerContext&) = delete;				// Disallow copying
};
}	 // namespace TPCW
#endif // RDMA_CLIENT_CONTEXT_H_
