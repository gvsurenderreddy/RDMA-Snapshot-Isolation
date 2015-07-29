/*
 *	CohortContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#ifndef COHORT_CONTEXT_H_
#define COHORT_CONTEXT_H_

#include "../../../../util/BaseContext.hpp"
#include "../../MemoryKeys.hpp"
#include "../../../../util/RDMACommon.hpp"
#include <iostream>

class CohortContext : public BaseContext {
public:
	std::string	server_address;	
	
	// memory handlers
	struct ibv_mr *send_message_mr;
	
	struct ibv_mr *send_data_mr;
	struct ibv_mr *recv_data_mr;
	

	// memory buffers
	struct MemoryKeys send_message_msg;
	
	int send_data_msg;
	int recv_data_msg;
	
	
	int register_memory();
	int destroy_context ();
};
#endif // COHORT_CONTEXT_H_