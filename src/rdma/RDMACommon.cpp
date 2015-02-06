/*
 *	RDMACommon.cpp
 *
 *	Created on: 26.01.2015
 *	Author: erfanz
 */

#include "RDMACommon.hpp"
#include "../util/utils.hpp"
#include <iostream>
#include <cstring>
using namespace std;

int RDMACommon::post_SEND (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length)
{
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	memset (&sge, 0, sizeof (sge));
	sge.addr		= local_buffer;
	sge.length		= length;
	sge.lkey		= local_mr->lkey;

	memset (&wr, 0, sizeof (wr));
	wr.next			= NULL;
	wr.wr_id		= 0;
	wr.sg_list		= &sge;
	wr.num_sge		= 1;
	wr.opcode		= IBV_WR_SEND;
	wr.send_flags	= IBV_SEND_SIGNALED;
	
	if (ibv_post_send(qp, &wr, &bad_wr)) {
		cerr << "Error, ibv_post_send() failed" << endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::post_RECEIVE (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length)
{
	struct ibv_recv_wr wr, *bad_wr;
	struct ibv_sge sge;

	memset (&sge, 0, sizeof (sge));
	sge.addr		= local_buffer;
	sge.length		= length;
	sge.lkey		= local_mr->lkey;

	memset (&wr, 0, sizeof (wr));
	wr.next			= NULL;
	wr.wr_id		= 0;
	wr.sg_list		= &sge;
	wr.num_sge		= 1;
	
	if (ibv_post_recv(qp, &wr, &bad_wr)) {
		cerr << "Error, ibv_post_recv() failed" << endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::post_RDMA_READ_WRT(enum ibv_wr_opcode opcode, struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer,
struct ibv_mr *peer_mr, uintptr_t peer_buffer, uint32_t length, bool signaled)
{
	struct ibv_sge sge;
	struct ibv_send_wr wr, *bad_wr;

	memset(&sge, 0, sizeof(sge));
	sge.addr				= local_buffer;
	sge.length				= length;
	sge.lkey	  			= local_mr->lkey;

	memset(&wr, 0, sizeof(wr));
	wr.wr_id      			= 0;
	wr.sg_list    			= &sge;
	wr.num_sge    			= 1;
	wr.opcode				= opcode;
	wr.wr.rdma.remote_addr	= peer_buffer;
	wr.wr.rdma.rkey       	= peer_mr->rkey;
	if (signaled)
		wr.send_flags 		= IBV_SEND_SIGNALED;
	else
		wr.send_flags 		= 0;
	
	if (ibv_post_send(qp, &wr, &bad_wr)) {
		cerr << "Error, ibv_post_send() failed" << endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::send_RDMA_FETCH_ADD(struct ibv_qp *qp, struct ibv_mr *local_mr, uint64_t local_buffer, 
struct ibv_mr *peer_mr, uint64_t peer_buffer, uint64_t addition, uint32_t length)
{
	struct ibv_sge sge;
	struct ibv_send_wr wr, *bad_wr = NULL;
	
	memset(&sge, 0, sizeof(sge));
	sge.addr 		= local_buffer;
	sge.length 		= length;
	sge.lkey 		= local_mr->lkey;
	
	memset(&wr, 0, sizeof(wr));
	wr.wr_id					= 0;
	wr.sg_list 					= &sge;
	wr.num_sge 					= 1;
	wr.opcode 					= IBV_WR_ATOMIC_FETCH_AND_ADD;
	wr.send_flags 				= IBV_SEND_SIGNALED;
	wr.wr.atomic.remote_addr	= peer_buffer;
	wr.wr.atomic.rkey        	= peer_mr->rkey;
	wr.wr.atomic.compare_add	= addition; /* value to be added to the remote address content */
	
	if (ibv_post_send(qp, &wr, &bad_wr)) {
		cerr << "Error, ibv_post_send() failed" << endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::post_RDMA_CMP_SWAP(struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer,
struct ibv_mr *peer_mr, uintptr_t peer_buffer, uint32_t length, uint64_t expected_value, uint64_t new_value)
{
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;
		
	memset(&sge, 0, sizeof(sge));
	sge.addr 		= local_buffer;
	sge.length 		= length;
	sge.lkey 		= local_mr->lkey;
	
	memset(&wr, 0, sizeof(wr));
	wr.wr_id					= 0;
	wr.opcode 					= IBV_WR_ATOMIC_CMP_AND_SWP;
	wr.sg_list 					= &sge;
	wr.num_sge 					= 1;
	wr.send_flags 				= IBV_SEND_SIGNALED;
	wr.wr.atomic.remote_addr	= peer_buffer;
	wr.wr.atomic.rkey        	= peer_mr->rkey;
	wr.wr.atomic.compare_add	= expected_value; /* expected value in remote address */
	wr.wr.atomic.swap        	= new_value; /* the value that remote address will be assigned to */
		
	if (ibv_post_send(qp, &wr, &bad_wr)) {
		cerr << "Error, ibv_post_send() failed" << endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::create_queuepair(struct ibv_context *ib_ctx, struct ibv_pd *pd, struct ibv_cq *cq, struct ibv_qp **qp)
{
	struct ibv_exp_device_attr dev_attr;
	struct ibv_exp_qp_init_attr	attr;
	
	memset(&dev_attr, 0, sizeof(dev_attr));
	dev_attr.comp_mask |= IBV_EXP_DEVICE_ATTR_EXT_ATOMIC_ARGS | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;
	if (ibv_exp_query_device(ib_ctx, &dev_attr)) {
		cerr << "ibv_exp_query_device failed" << endl;
		return -1;
	}

	memset(&attr, 0, sizeof(struct ibv_exp_qp_init_attr));
	attr.pd = pd;
	attr.send_cq = cq;
	attr.recv_cq = cq;
	attr.cap.max_send_wr  = 1;
	attr.cap.max_send_sge = 1;
	attr.cap.max_inline_data = 0;
	attr.cap.max_recv_wr  = 1;
	attr.cap.max_recv_sge = 1;
	//attr.max_atomic_arg = pow(2,5);
	attr.max_atomic_arg = 32;
	attr.exp_create_flags = IBV_EXP_QP_CREATE_ATOMIC_BE_REPLY;
	attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS | IBV_EXP_QP_INIT_ATTR_PD;
	attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_ATOMICS_ARG;
	attr.srq = NULL;
	attr.qp_type = IBV_QPT_RC;
	
	(*qp) = ibv_exp_create_qp(ib_ctx, &attr);
   	if (!(*qp))
	{
		cerr << "failed to create QP" << endl;
		return -1;
	}
	DEBUG_COUT ("QP was created, QP number=0x" << (*qp)->qp_num);
	//fprintf (stdout, "QP was created, QP number=0x%x\n", (*qp)->qp_num);
	
	return 0;
}

int RDMACommon::modify_qp_to_init (int ib_port, struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_INIT;
	attr.port_num = ib_port;
	attr.pkey_index = 0;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to INIT" << endl;
	return rc;
}

int RDMACommon::modify_qp_to_rtr (int ib_port, struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid, uint8_t * dgid)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = remote_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 4;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dlid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = ib_port;

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
		IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to RTR" << endl;
	return rc;
}

int RDMACommon::modify_qp_to_rts (struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 4;
	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		cerr << "failed to modify QP state to RTS" << endl;
	return rc;
}

int RDMACommon::poll_completion(struct ibv_cq* cq)
{
	struct ibv_wc wc;
	int ne;
	do {
		wc.status = IBV_WC_SUCCESS;
		ne = ibv_poll_cq(cq, 1, &wc);
		if(wc.status != IBV_WC_SUCCESS) {
			cerr << "RDMA completion event in CQ with error!" << endl;
			return -1;
		}
	}while(ne==0);

	if(ne<0) {
		cerr << "RDMA polling from CQ failed!" << endl;
		return -1;
	}
	return 0;
}