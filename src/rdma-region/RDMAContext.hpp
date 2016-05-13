/*
 *	RDMAContext.hpp
 *
 *	Created on: 19.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef RDMA_CONTEXT_H_
#define RDMA_CONTEXT_H_

#include <cstdint>	// for uintX_t
#include <infiniband/verbs.h>
#include <ostream>	// std::ostream

class RDMAContext {
private:
	std::ostream &os_;
	uint8_t	ib_port_;
	struct	ibv_device_attr device_attr_;
	struct	ibv_port_attr port_attr_;				/* IB port attributes */
	struct	ibv_context *ib_ctx_;					/* device handle */
	struct	ibv_pd *pd_;							/* PD handle */
	struct	ibv_cq *send_cq_;						/* send CQ handle */
	struct	ibv_cq *recv_cq_;						/* receive CQ handle */
	struct	ibv_comp_channel *send_comp_channel_;	/* send CQ channel */
	struct	ibv_comp_channel *recv_comp_channel_;	/* receive CQ channel */

public:
	RDMAContext(std::ostream &os, uint8_t ib_port);
	uint8_t getIbPort();
	ibv_context* getIbCtx();
	ibv_pd* getPd();
	ibv_cq* getSendCq();
	ibv_cq* getRecvCq();
	ibv_comp_channel* getSendCompletionChannel();
	ibv_comp_channel* getRecvCompletionChannel();

	ibv_port_attr getPortAttr();
	~RDMAContext();
};
#endif /* RDMA_CONTEXT_H_ */
