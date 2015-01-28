/*
 *	RDMACommon.cpp
 *
 *	Created on: 26.01.2015
 *	Author: erfanz
 */

#ifndef RDMACOMMON_H_
#define RDMACOMMON_H_

#include <infiniband/verbs.h>

class RDMACommon {
public:

	/******************************************************************************
	* Function: post_SEND
	*
	* Input
	* res pointer to resources structure
	* opcode IBV_WR_SEND, IBV_WR_RDMA_READ or IBV_WR_RDMA_WRITE
	*
	* Returns
	* 0 on success, error code on failure
	*
	* Description
	* This function will create and post a send work request
	******************************************************************************/
	static int post_SEND (struct ibv_qp *qp, struct ibv_mr *local_mr, uint64_t local_buffer, uint32_t length);
	
	
	/******************************************************************************
	* Function: post_RECEIVE
	*
	* Input
	* res pointer to resources structure
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, error code on failure
	*
	* Description
	*
	******************************************************************************/
	static int post_RECEIVE (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length);
	
	static int create_queuepair(struct ibv_context *context, struct ibv_pd *pd, struct ibv_cq *cq, struct ibv_qp **qp);
	
	
	/******************************************************************************
	* Function: modify_qp_to_init
	*
	* Input
	* qp QP to transition
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, ibv_modify_qp failure code on failure
	*
	* Description
	* Transition a QP from the RESET to INIT state
	******************************************************************************/
	static int modify_qp_to_init (int ib_port, struct ibv_qp *qp);
	
	
	/******************************************************************************
	* Function: modify_qp_to_rtr
	*
	* Input
	* qp QP to transition
	* remote_qpn remote QP number
	* dlid destination LID
	* dgid destination GID (mandatory for RoCEE)
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, ibv_modify_qp failure code on failure
	*
	* Description
	* Transition a QP from the INIT to RTR state, using the specified QP number
	******************************************************************************/
	static int modify_qp_to_rtr (int ib_port, struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid, uint8_t * dgid);
	
	
	/******************************************************************************
	* Function: modify_qp_to_rts
	*
	* Input
	* qp QP to transition
	*
	* Output
	* none
	*
	* Returns
	* 0 on success, ibv_modify_qp failure code on failure
	*
	* Description
	* Transition a QP from the RTR to RTS state
	******************************************************************************/
	static int modify_qp_to_rts (struct ibv_qp *qp);
	
	
	/******************************************************************************
	* Function: poll_completion
	*
	* Input
	* pointer to the completion queue
	*
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	* Poll the completion queue for a single event. This function will continue to
	* poll the queue indefinitely
	* (TODO: might need to be fixed, such that it waits for a specific amount of time).
	*
	******************************************************************************/
	static int poll_completion(struct ibv_cq* cq);
	
};
#endif /* RDMACOMMON_H_ */
