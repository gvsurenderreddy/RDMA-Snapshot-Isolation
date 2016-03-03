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
#include <string>	// for std::string
#include <infiniband/verbs.h>	// for ibv_qp
#include <cstdint>	// uintX_t

namespace TPCC{
class ServerContext {
private:
	int 		sockfd_;
	std::string	serverAddress_;
	uint16_t	tcpPort_;
	uint8_t 	ibPort_;
	unsigned	instanceNum_;
	struct	ibv_qp	*qp_;

	// RDMA region for storing remote memory keys
	RDMARegion<ServerMemoryKeys> *peerMemoryKeys_;

public:
	ServerContext(const int sockfd, const std::string &serverAddress, const uint16_t tcpPort, const uint8_t ibPort, const unsigned instanceNum, RDMAContext &context);
	~ServerContext();
	std::string getServerAddress() const;
	uint16_t getTcpPort() const;
	int getSockFd() const;
	ibv_qp* getQP() const;
	RDMARegion<ServerMemoryKeys>* getRemoteMemoryKeys();

	void activateQueuePair(RDMAContext &context);

	ServerContext& operator=(const ServerContext&) = delete;	// Disallow copying
	ServerContext(const ServerContext&) = delete;				// Disallow copying
};
}	// namespace TPCC
#endif // SERVER_CONTEXT_H_