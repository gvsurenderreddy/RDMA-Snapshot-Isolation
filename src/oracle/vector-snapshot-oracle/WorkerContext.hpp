/*
 * WorkerContext.hpp
 *
 *  Created on: Feb 28, 2016
 *      Author: erfanz
 */

#ifndef SRC_ORACLE_WORKERCONTEXT_HPP_
#define SRC_ORACLE_WORKERCONTEXT_HPP_

#include "../../rdma-region/RDMAContext.hpp"
#include "../../rdma-region/RDMARegion.hpp"
#include "../../basic-types/PrimitiveTypes.hpp"
#include "OracleMemoryKeys.hpp"
#include <iostream>

class WorkerContext {
public:
	std::ostream 	&os_;
	int 			sockfd_;
	std::string		clientIp_;
	int				clientPort_;
	struct	ibv_qp	*qp_;	/* QP handle */
	RDMARegion<OracleMemoryKeys> *memoryKeysMessage_;


	WorkerContext(std::ostream &os, int sockfd, RDMAContext &context);
	RDMARegion<OracleMemoryKeys>* getMemoryKeysMessage();
	int getSockFd() const;
	ibv_qp* getQP() const;
	void activateQueuePair(RDMAContext &context);
	friend std::ostream& operator<<(std::ostream& os, const WorkerContext& ctx) {
		os << "IP: " << ctx.clientIp_ << ", port: " << ctx.clientPort_  << ", sock: " << ctx.sockfd_;
		return os;
	}
	~WorkerContext();
};

#endif /* SRC_ORACLE_WORKERCONTEXT_HPP_ */
