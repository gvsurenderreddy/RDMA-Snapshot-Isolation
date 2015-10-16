/*
 *	IPCohortContext.hpp
 *
 *	Created on: 26.Mar.2015
 *	Author: Erfan Zamanian
 */

#ifndef IP_COHORT_CONTEXT_H_
#define IP_COHORT_CONTEXT_H_

#include "../../../../util/BaseContext.hpp"
#include "../../../../util/RDMACommon.hpp"
#include <iostream>

class IPCohortContext : public BaseContext {
public:
	std::string	server_address;	
	
	int send_data_msg;
	int recv_data_msg;
	
	int create_context ();
	int register_memory();
	int destroy_context ();
};
#endif // IP_COHORT_CONTEXT_H_