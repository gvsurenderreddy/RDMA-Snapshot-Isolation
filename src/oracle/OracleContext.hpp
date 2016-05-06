/*
 * OracleContext.hpp
 *
 *  Created on: Feb 29, 2016
 *      Author: erfanz
 */

#ifndef SRC_ORACLECONTEXT_HPP_
#define SRC_ORACLECONTEXT_HPP_

#include "OracleMemoryKeys.hpp"
#include "../rdma-region/RDMARegion.hpp"
#include <set>

class OracleContext {
private:
	std::ostream &os_;
	int 		sockfd_;
	uint16_t	tcpPort_;
	uint8_t 	ibPort_;
	struct	ibv_qp	*qp_;
	std::set<primitive::client_id_t> clientIDsInSnapshot_;		// only used when config::SNAPSHOT_ACQUISITION_TYPE is set to OnLY_READ_SET

	// RDMA region for storing remote memory keys
	RDMARegion<OracleMemoryKeys> *peerMemoryKeys_;


public:
	OracleContext(std::ostream &os, const int sockfd, const uint16_t tcpPort, const uint8_t ibPort, RDMAContext &context);
	~OracleContext();
	uint16_t getTcpPort() const;
	int getSockFd() const;
	ibv_qp* getQP() const;
	std::set<primitive::client_id_t> getClientIDsInSnapshot() const;
	void insertClientIDIntoSnapshot(primitive::client_id_t);
	void clearSnapshot();

	RDMARegion<OracleMemoryKeys>* getRemoteMemoryKeys();
	void activateQueuePair(RDMAContext &context);

	OracleContext& operator=(const OracleContext&) = delete;	// Disallow copying
	OracleContext(const OracleContext&) = delete;				// Disallow copying
};

#endif /* SRC_ORACLECONTEXT_HPP_ */
