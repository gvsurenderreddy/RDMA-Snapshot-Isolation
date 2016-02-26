/*
 * RDMARegion.hpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#ifndef SRC_RDMA_REGION_RDMAREGION_HPP_
#define SRC_RDMA_REGION_RDMAREGION_HPP_

#include "MemoryHandler.hpp"
#include "../util/utils.hpp"
#include "../util/RDMACommon.hpp"
#include "RDMAContext.hpp"


template <typename T> class RDMARegion {
private:
	T				*region_;
	size_t			regionSize_;
	struct ibv_mr	*rdmaHandler_;

public:
	RDMARegion(size_t regionSize, RDMAContext &baseContext, int mrFlags);
	T* getRegion();
	size_t getRegionSize();
	size_t getRegionSizeInByte();
	ibv_mr* getRDMAHandler();
	void getMemoryHandler(MemoryHandler<T>& mh);
	virtual ~RDMARegion();
};


/* Note that template classes must be implemented in the header file, since the compiler
 * needs to have access to the implementation of the methods when instantiating the template class at compile time.
 * For more information, please refer to:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 */

template <typename T>
RDMARegion<T>::RDMARegion(size_t regionSize, RDMAContext &context, int mrFlags)
: regionSize_(regionSize){

	region_ = new T[regionSize_];
	TEST_Z(rdmaHandler_ = ibv_reg_mr(context.getPd(), region_, sizeof(T) * regionSize_, mrFlags));
}

template <typename T>
T* RDMARegion<T>::getRegion(){
	return region_;
}

template <typename T>
size_t RDMARegion<T>::getRegionSize(){
	return regionSize_;
}

template <typename T>
size_t RDMARegion<T>::getRegionSizeInByte(){
	return sizeof(T) * regionSize_;
}

template <typename T>
ibv_mr* RDMARegion<T>::getRDMAHandler(){
	return rdmaHandler_;
}

template <typename T>
void RDMARegion<T>::getMemoryHandler(MemoryHandler<T>& mh){
	mh.rdmaHandler_ = *rdmaHandler_;
	mh.regionSize_ = regionSize_;
	mh.region_ = region_;
}

template <typename T>
RDMARegion<T>::~RDMARegion() {
	if (rdmaHandler_)
		TEST_NZ (ibv_dereg_mr (rdmaHandler_));
	delete[] region_;
}

#endif /* SRC_RDMA_REGION_RDMAREGION_HPP_ */
