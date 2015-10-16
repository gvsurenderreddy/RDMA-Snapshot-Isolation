/*
 *	ServerContext.cpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#include "ServerContext.hpp"
#include "../../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int ServerContext::register_memory() {
	int mr_flags	=
		IBV_ACCESS_LOCAL_WRITE
			| IBV_ACCESS_REMOTE_READ
				| IBV_ACCESS_REMOTE_WRITE
					| IBV_ACCESS_REMOTE_ATOMIC;
	
		
	TEST_Z(local_mr	= ibv_reg_mr(pd, local_buffer, ARRAY_SIZE * sizeof(struct MyStruct), mr_flags));
	TEST_Z(send_message_mr	= ibv_reg_mr(pd, &send_message_msg, sizeof(struct ibv_mr), mr_flags));
	
	return 0;
}

int ServerContext::destroy_context () {
	if (qp)
		TEST_NZ(ibv_destroy_qp (qp));
	
	if (local_mr) TEST_NZ (ibv_dereg_mr (local_mr));

	if (send_message_mr) TEST_NZ (ibv_dereg_mr (send_message_mr));
	
	if (cq)
		TEST_NZ (ibv_destroy_cq (cq));
	
	if (pd)
		TEST_NZ (ibv_dealloc_pd (pd));
	
	if (ib_ctx)
		TEST_NZ (ibv_close_device (ib_ctx));

	if (sockfd >= 0){
		TEST_NZ (close (sockfd));
	}
	return 0;
}