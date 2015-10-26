/*
 *	ServerContext.cpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#include "ServerContext.hpp"
#include "../benchmark-config.hpp"
#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int ServerContext::register_memory() {
	int mr_flags	=
		IBV_ACCESS_LOCAL_WRITE
			| IBV_ACCESS_REMOTE_READ
				| IBV_ACCESS_REMOTE_WRITE
					| IBV_ACCESS_REMOTE_ATOMIC;

	TEST_Z(send_message_mr	= ibv_reg_mr(pd, &send_message_msg, sizeof(struct MemoryKeys), mr_flags));
	TEST_Z(global_buffer_mr	= ibv_reg_mr(pd, global_buffer, benchmark_config::SERVER_REGION_WORDS * sizeof(uint64_t), mr_flags));
	return 0;
}

int ServerContext::destroy_context () {
	if (qp) 				TEST_NZ(ibv_destroy_qp (qp));
	if (send_message_mr)	TEST_NZ (ibv_dereg_mr (send_message_mr));
	if (global_buffer_mr)	TEST_NZ (ibv_dereg_mr (global_buffer_mr));
	this->BaseContext::destroy_context();
	return 0;
}
