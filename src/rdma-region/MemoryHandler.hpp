/*
 * MemoryHandler.hpp
 *
 *  Created on: Feb 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_RDMA_REGION_MEMORYHANDLER_HPP_
#define SRC_RDMA_REGION_MEMORYHANDLER_HPP_

#include "../util/utils.hpp"
#include <infiniband/verbs.h>
#include <iostream>

template <class T>  class MemoryHandler{
public:
	struct ibv_mr	rdmaHandler_;	// for remote access
	T				*region_;		// for local access
	size_t			regionSize_;
	size_t			regionSizeInBytes_;

	bool isAddressInRange(uintptr_t lookupAddress) {
		if ((lookupAddress < (uintptr_t) rdmaHandler_.addr) || (lookupAddress >= (uintptr_t)rdmaHandler_.addr + regionSizeInBytes_)){
			PRINT_CERR("MemoryHandler", __func__, "Accessing outside the region: " << lookupAddress << " NOT IN ["
					<< (uintptr_t) rdmaHandler_.addr << ", " << (uintptr_t)rdmaHandler_.addr + regionSizeInBytes_ << ") range. Region size: " << (int)regionSize_);
			return false;
		}
		else return true;
	}
};


#endif /* SRC_RDMA_REGION_MEMORYHANDLER_HPP_ */
