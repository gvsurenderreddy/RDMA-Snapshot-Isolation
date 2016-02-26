/*
 * ClientContext.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_
#define SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_

#include "../../../rdma-region/RDMAContext.hpp"

class ClientContext {
public:
	int 			sockfd_;
	struct	ibv_qp	*qp_;	/* QP handle */

	ClientContext(int sockfd, RDMAContext &context);
	int getSockFd() const;
	ibv_qp* getQP() const;
	void activateQueuePair(RDMAContext &context);
	~ClientContext();
};

#endif /* SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_ */
