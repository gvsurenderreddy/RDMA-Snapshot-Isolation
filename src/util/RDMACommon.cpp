/*
 *	RDMACommon.cpp
 *
 *	Created on: 26.Jan.2015
 *	Author: Erfan Zamanian
 */

#include "RDMACommon.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

#define CLASS_NAME "RDMACommon"


int RDMACommon::post_SEND (struct ibv_qp *qp, struct ibv_mr *local_mr, uintptr_t local_buffer, uint32_t length, bool signaled)
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
	if (signaled)
		wr.send_flags 		= IBV_SEND_SIGNALED;
	else
		wr.send_flags 		= 0;
	
	if (ibv_post_send(qp, &wr, &bad_wr)) {
		std::cerr << "Error, ibv_post_send() failed" << std::endl;
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
		std::cerr << "Error, ibv_post_recv() failed" << std::endl;
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
		std::cerr << "Error, ibv_post_send() failed" << std::endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::post_RDMA_FETCH_ADD(struct ibv_qp *qp, struct ibv_mr *local_mr, uint64_t local_buffer, 
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
		std::cerr << "Error, ibv_post_send() failed" << std::endl;
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
		std::cerr << "Error, ibv_post_send() failed" << std::endl;
		return -1;
	}	
	return 0;
}

int RDMACommon::create_queuepair(struct ibv_context *ib_ctx, struct ibv_pd *pd, struct ibv_cq *send_cq, struct ibv_cq *recv_cq, struct ibv_qp **qp)
{
	struct ibv_exp_device_attr dev_attr;
	struct ibv_exp_qp_init_attr	attr;
	
	memset(&dev_attr, 0, sizeof(dev_attr));
	dev_attr.comp_mask |= IBV_EXP_DEVICE_ATTR_EXT_ATOMIC_ARGS | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;
	if (ibv_exp_query_device(ib_ctx, &dev_attr)) {
		std::cerr << "ibv_exp_query_device failed" << std::endl;
		return -1;
	}

	memset(&attr, 0, sizeof(struct ibv_exp_qp_init_attr));
	attr.pd = pd;
	attr.send_cq = send_cq;
	attr.recv_cq = recv_cq;
	attr.sq_sig_all = 0;	// In every WR, it must be decided whether to generate a WC or not
	attr.cap.max_send_wr  = RDMA_MAX_WR;
	attr.cap.max_send_sge = RDMA_MAX_SGE;
	attr.cap.max_inline_data = 0;
	attr.cap.max_recv_wr  = 2 * RDMA_MAX_WR;
	attr.cap.max_recv_sge = RDMA_MAX_SGE;
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
		std::cerr << "failed to create QP" << std::endl;
		return -1;
	}
	DEBUG_COUT (CLASS_NAME, __func__, "[Conn] QP created, QP number=0x" << (*qp)->qp_num);
	return 0;
}

int RDMACommon::modify_qp_to_init (uint8_t ib_port, struct ibv_qp *qp)
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
		std::cerr << "failed to modify QP state to INIT" << std::endl;
	return rc;
}

int RDMACommon::modify_qp_to_rtr (uint8_t ib_port, struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid)
{
	struct ibv_qp_attr attr;
	int flags;
	int rc;
	memset (&attr, 0, sizeof (attr));
	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_4096;
	attr.dest_qp_num = remote_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 16;
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
		std::cerr << "failed to modify QP state to RTR" << std::endl;
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
	attr.max_rd_atomic = 16;
	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
	rc = ibv_modify_qp (qp, &attr, flags);
	if (rc)
		std::cerr << "failed to modify QP state to RTS" << std::endl;
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
			std::cerr << "RDMA completion event in CQ with error!" << std::endl;
			std::cerr << "wc_status: " << wc.status << std::endl;

			return -1;
		}
	}while(ne==0);

	if(ne<0) {
		std::cerr << "RDMA polling from CQ failed!" << std::endl;
		return -1;
	}
	return 0;
}

int RDMACommon::poll_completion(struct ibv_cq* cq, uint32_t &returned_qp_num)
{
	struct ibv_wc wc;
	int ne;
	do {
		wc.status = IBV_WC_SUCCESS;
		ne = ibv_poll_cq(cq, 1, &wc);
		if(wc.status != IBV_WC_SUCCESS) {
			std::cerr << "RDMA completion event in CQ with error!" << std::endl;
			std::cerr << "wc_status: " << wc.status << std::endl;

			return -1;
		}
	}while(ne==0);

	if(ne<0) {
		std::cerr << "RDMA polling from CQ failed!" << std::endl;
		return -1;
	}
	returned_qp_num = wc.qp_num;
	return 0;
}

int RDMACommon::event_based_poll_completion(struct ibv_comp_channel *comp_channel, struct ibv_cq *cq) {
	/* The following code will be called each time you need to read a Work Completion */
	struct ibv_cq *ev_cq;
	void *ev_ctx;
	int ne;
	struct ibv_wc wc;

	/* Wait for the Completion event */
	TEST_NZ (ibv_get_cq_event(comp_channel, &ev_cq, &ev_ctx));

	/* Ack the event */
	ibv_ack_cq_events(ev_cq, 1);

	/* Request notification upon the next completion event */
	TEST_NZ (ibv_req_notify_cq(ev_cq, 0));

	/* Empty the CQ: poll all of the completions from the CQ (if any exist) */
	do {
	    ne = ibv_poll_cq(cq, 1, &wc);
	    if (ne < 0) {
	            fprintf(stderr, "Failed to poll completions from the CQ: ret = %d\n", ne);
	            return -1;
	    }
	    /* there may be an extra event with no completion in the CQ */
	    if (ne == 0)
	            continue;

	    if (wc.status != IBV_WC_SUCCESS) {
	            fprintf(stderr, "Completion with status 0x%x was found\n", wc.status);
	            return -1;
	    }
	} while (ne);
	return 0;
}

int RDMACommon::build_connection(int ib_port, struct ibv_context** ib_ctx,
struct ibv_port_attr* port_attr, struct ibv_pd **pd, struct ibv_cq **send_cq, struct ibv_cq **recv_cq,
struct	ibv_comp_channel **send_comp_channel, struct ibv_comp_channel **recv_comp_channel, int cq_size)
{
	struct	ibv_device **dev_list = NULL;
	struct	ibv_device *ib_dev = NULL;
	int		num_devices;
	//struct	ibv_comp_channel *comp_channel;

	// get device names in the system
	TEST_Z(dev_list = ibv_get_device_list (&num_devices));
	TEST_Z(num_devices); // if there isn't any IB device in host

	// select the first device
	//const char *dev_name = strdup (ibv_get_device_name (dev_list[0]));
	TEST_Z(ib_dev = dev_list[0]);	// if the device wasn't found in host
	
	TEST_Z(*ib_ctx = ibv_open_device (ib_dev));		// get device handle

	// We are now done with device list, free it
	ibv_free_device_list (dev_list);
	dev_list = NULL;
	ib_dev = NULL;
	
	TEST_NZ (ibv_query_port (*ib_ctx, ib_port, port_attr));

	TEST_Z(*pd = ibv_alloc_pd (*ib_ctx));		// allocate Protection Domain

	// Create completion channel and completion queue for send queue
	TEST_Z(*send_comp_channel = ibv_create_comp_channel(*ib_ctx));
	TEST_Z(*send_cq = ibv_create_cq (*ib_ctx, cq_size, NULL, *send_comp_channel, 0));
	TEST_NZ (ibv_req_notify_cq(*send_cq, 0));
	
	// Create completion channel and completion queue for receive queue
	TEST_Z(*recv_comp_channel = ibv_create_comp_channel(*ib_ctx));
	TEST_Z(*recv_cq = ibv_create_cq (*ib_ctx, cq_size, NULL, *recv_comp_channel, 0));
	TEST_NZ (ibv_req_notify_cq(*recv_cq, 0));
	
	return 0;
}

int RDMACommon::connect_qp (struct ibv_qp **qp, uint8_t ib_port, uint16_t lid, int sockfd)
{
	struct CommExchData local_con_data, remote_con_data, tmp_con_data;
	char temp_char;
	union ibv_gid my_gid;
	
	memset (&my_gid, 0, sizeof my_gid);
	
	// exchange using TCP sockets info required to connect QPs
	local_con_data.qp_num	= htonl ((*qp)->qp_num);
	local_con_data.lid		= htons (lid);
	
	memcpy (local_con_data.gid, &my_gid, sizeof my_gid);
	
	TEST_NZ (utils::sock_sync_data(sockfd, sizeof (struct CommExchData), (char *) &local_con_data, (char *) &tmp_con_data));

	remote_con_data.qp_num	= ntohl (tmp_con_data.qp_num);
	remote_con_data.lid		= ntohs (tmp_con_data.lid);
	memcpy (remote_con_data.gid, tmp_con_data.gid, 16);
	
	// save the remote side attributes, we will need it for the post SR
	// this line might be needed:
	// ctx->remote_props = remote_con_data;
	// fprintf (stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
	// fprintf (stdout, "Remote LID = 0x%x\n", remote_con_data.lid);
	// modify the QP to init
	TEST_NZ(RDMACommon::modify_qp_to_init (ib_port, *qp));
	
	// modify the QP to RTR
	TEST_NZ(RDMACommon::modify_qp_to_rtr (ib_port, *qp, remote_con_data.qp_num, remote_con_data.lid));
	
	// modify the QP to RTS
	TEST_NZ(RDMACommon::modify_qp_to_rts (*qp));
	

	// sync to make sure that both sides are in states that they can connect to prevent packet loss
	TEST_NZ(utils::sock_sync_data (sockfd, 1, "Q", &temp_char));	// just send a dummy char back and forth
	//TEST_NZ (utils::sock_sync (sockfd));	// just send a dummy char back and forth
	return 0;
}
