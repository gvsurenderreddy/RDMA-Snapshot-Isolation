/*
 * ClientContext.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_
#define SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_

#include "../tables/IndexRequestMessage.hpp"
#include "../tables/IndexResponseMessage.hpp"
#include "../../../rdma-region/RDMAContext.hpp"
#include "../../../rdma-region/RDMARegion.hpp"

namespace TPCC{
class ClientContext {
public:
	int 			sockfd_;
	struct	ibv_qp	*qp_;	/* QP handle */

	RDMARegion<TPCC::IndexRequestMessage>	*indexRequestMessage_;
	RDMARegion<TPCC::IndexResponseMessage>	*indexResponseMessage_;

	ClientContext(int sockfd, RDMAContext &context);
	int getSockFd() const;
	ibv_qp* getQP() const;
	void activateQueuePair(RDMAContext &context);
	RDMARegion<TPCC::IndexRequestMessage>* getIndexRequestMessageRegion() const;
	RDMARegion<TPCC::IndexResponseMessage>* getIndexResponseMessageRegion() const;
	~ClientContext();
};
}	// namespace TPCC
#endif /* SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_ */
