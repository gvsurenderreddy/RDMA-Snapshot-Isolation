/*
 * ClientContext.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_
#define SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_

#include "../index-messages/IndexRequestMessage.hpp"
#include "../index-messages/IndexResponseMessage.hpp"
#include "../index-messages/CustomerNameIndexRespMsg.hpp"
#include "../index-messages/LargestOrderForCustomerIndexRespMsg.hpp"
#include "../index-messages/OldestUndeliveredOrderIndexResMsg.hpp"
#include "../index-messages/Last20OrdersIndexResMsg.hpp"
#include "../../../../rdma-region/RDMAContext.hpp"
#include "../../../../rdma-region/RDMARegion.hpp"

namespace TPCC{
class ClientContext {
public:
	std::ostream &os_;
	int 			sockfd_;
	struct	ibv_qp	*qp_;	/* QP handle */

	RDMARegion<TPCC::IndexRequestMessage>	*indexRequestMessage_;
	RDMARegion<TPCC::IndexResponseMessage>	*indexResponseMessage_;
	RDMARegion<TPCC::CustomerNameIndexRespMsg>	*customerNameIndexResponseMessage_;
	RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>	*largestOrderIndexResponseMessage_;
	RDMARegion<TPCC::Last20OrdersIndexResMsg>	*last20OrdersIndexResponseMessage_;
	RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg>	*oldestUndeliveredOrderIndexResponseMessage_;

	ClientContext(std::ostream &os, int sockfd, RDMAContext &context);
	int getSockFd() const;
	ibv_qp* getQP() const;
	void activateQueuePair(RDMAContext &context);
	RDMARegion<TPCC::IndexRequestMessage>* getIndexRequestMessage() const;
	RDMARegion<TPCC::IndexResponseMessage>* getIndexResponseMessage() const;
	RDMARegion<TPCC::CustomerNameIndexRespMsg>* getCustomerNameIndexResponseMessage() const;
	RDMARegion<TPCC::LargestOrderForCustomerIndexRespMsg>* getLargestOrderForCustomerIndexResponseMessage() const;
	RDMARegion<TPCC::Last20OrdersIndexResMsg>* getLast20OrdersIndexResponseMessage() const;
	RDMARegion<TPCC::OldestUndeliveredOrderIndexResMsg>* getOldestUndeliveredOrderIndexResponseMessage() const;


	~ClientContext();
};
}	// namespace TPCC
#endif /* SRC_BENCHMARKS_TPC_C_SERVER_CLIENTCONTEXT_HPP_ */
