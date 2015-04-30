/*
 *	IPCohortContext.cpp
 *
 *	Created on: 26.Mar.2015
 *	Author: erfanz
 */

#include "IPCohortContext.hpp"
#include "../../../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>

int IPCohortContext::create_context() {
	return 0;
}

int IPCohortContext::register_memory() {
	return 0;
}

int IPCohortContext::destroy_context () {
	if (sockfd >= 0){
		TEST_NZ (close (sockfd));
	}
}