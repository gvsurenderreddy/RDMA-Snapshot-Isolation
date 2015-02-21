/*
 *	RDMACommon.hpp
 *
 *	Created on: 26.Jan.2015
 *	Author: erfanz
 */

#ifndef RDMACOMMON_H_
#define RDMACOMMON_H_

#include <infiniband/verbs.h>


#define RDMA_MAX_WR 1000
#define RDMA_MAX_SGE 1


class RDMACommon {
private:
	
	/* structure to exchange data which is needed to connect the QPs */
	struct CommExchData
	{
		uint32_t qp_num;		/* QP number */
		uint16_t lid;			/* LID of the IB port */
		uint8_t gid[16];		/* gid */
	} __attribute__ ((packed));
	
public:	
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
	static int post_SEND (struct ibv_qp *qp, struct ibv_mr *local_mr, uint64_t local_buffer, uint32_t length, bool signaled);
	
	
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
	
	
	/******************************************************************************
	* Prepares and executes an RDMA READ/WRITE operation, depending on the input opcode 
	*
	* @param	conn			the connection on which 
	* @param	opcode			the opcode used for the operation (IBV_WR_RDMA_WRITE or IBV_WR_RDMA_READ)
	* @param	local_buffer	the address of the local buffer to read from/write to
	* @param	remote_buffer	the address of the remote buffer to write to/read from
	* @param	lcoal_mr		the pointer to the local ibv_mr
	* @param	peer_mr			the pointer to the remote ibv_mr
	* @param	length			the length of the reading/writing
	* @return	0 or 1			In case of success 0, and in case of failure -1
	******************************************************************************/
	static int post_RDMA_READ_WRT(enum ibv_wr_opcode opcode, struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer,
	struct ibv_mr *peer_mr, uintptr_t peer_buffer, uint32_t length, bool signaled);
	
	
	/******************************************************************************
	* Prepares and executes an RDMA Compare and Swap operation 
	*
	* @param	conn			the connection on which 
	* @param	local_buffer	the address of the local buffer to read from/write to
	* @param	remote_buffer	the address of the remote buffer to write to/read from
	* @param	length			the length of the reading/writing
	* @param	old_value		the value at the remote buffer to be compared with
	* @param	new_value		the value to be set at the remote buffer
	* @return	0 or 1			In case of success 0, and in case of failure -1
	******************************************************************************/
	static int post_RDMA_CMP_SWAP(struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer,
	struct ibv_mr *peer_mr, uintptr_t peer_buffer, uint32_t length, uint64_t expected_value, uint64_t new_value);
		
		
	/******************************************************************************
	* Prepares and executes an RDMA Fetch and Add operation 
	*
	* @param	conn			the connection on which 
	* @param	local_buffer	the address of the local buffer to read from/write to
	* @param	remote_buffer	the address of the remote buffer to write to/read from
	* @param	length			the length of the reading/writing
	* @param	lcoal_mr		the pointer to the local ibv_mr
	* @param	peer_mr			the pointer to the remote ibv_mr
	* @return	0 or 1			In case of success 0, and in case of failure -1
	******************************************************************************/
	static int post_RDMA_FETCH_ADD(struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, 
	struct ibv_mr *peer_mr, uintptr_t peer_buffer, uint64_t addition, uint32_t length);
	
	
	static int create_queuepair(struct ibv_context *context, struct ibv_pd *pd, struct ibv_cq *cq, struct ibv_qp **qp);
	
	
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
	
	static int build_connection(int ib_port, struct ibv_context** ib_ctx,
	struct ibv_port_attr* port_attr, struct ibv_pd **pd, struct ibv_cq **cq, int cq_size);
	
	
	/******************************************************************************
	* Function: connect_qp
	*
	* Input
	* - qp:			double pointer to the Queue Pair
	* - ib_port:	IB port number	 
	* - lid:		lid of the IB port (can be extracted from ibv_port_attr)		
	* - sockfd:		socket descriptor of the connection
	*	
	* Returns
	* 0 on success, 1 on failure
	*
	* Description
	* Connect the QP. Transition the server side to RTR, sender side to RTS
	*
	******************************************************************************/
	static int connect_qp (struct ibv_qp **qp, int ib_port, uint16_t lid, int sockfd);
};
#endif /* RDMACOMMON_H_ */
