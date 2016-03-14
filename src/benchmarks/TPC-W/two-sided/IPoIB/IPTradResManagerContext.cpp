/*
 *	IPTradResManagerContext.cpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#include "../../two-sided/IPoIB/IPTradResManagerContext.hpp"

#include "../../util/utils.hpp"
#include <unistd.h>		// for close
#include <iostream>


int IPTradResManagerContext::register_memory() {
	return 0;
}

int IPTradResManagerContext::destroy_context () {
	if (sockfd >= 0)	TEST_NZ (close (sockfd));
	
}