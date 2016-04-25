/*
 * MultiValueHashIndex.hpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#ifndef SRC_INDEX_HASH_MULTIVALUEHASHINDEX_HPP_
#define SRC_INDEX_HASH_MULTIVALUEHASHINDEX_HPP_

#include "HashIndex.hpp"
#include <vector>

template <class KeyT, class ValueT>
class MultiValueHashIndex : public HashIndex<KeyT, std::vector<ValueT> > {

typedef HashIndex<KeyT, std::vector<ValueT> > BaseClass;

public:
	MultiValueHashIndex();
	virtual ~MultiValueHashIndex();

	void append(const KeyT &k, const ValueT &v);
	ValueT getMiddle(const KeyT &k);
};


/* Note that template classes must be implemented in the header file, since the compiler
 * needs to have access to the implementation of the methods when instantiating the template class at compile time.
 * For more information, please refer to:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 */


template <class KeyT, class ValueT>
MultiValueHashIndex<KeyT, ValueT>::MultiValueHashIndex() {
	// TODO Auto-generated constructor stub

}
template <class KeyT, class ValueT>
MultiValueHashIndex<KeyT, ValueT>::~MultiValueHashIndex() {
	// TODO Auto-generated destructor stub
}

template <class KeyT, class ValueT>
void MultiValueHashIndex<KeyT, ValueT>::append(const KeyT &k, const ValueT &v) {
	std::lock_guard<std::mutex> lock(BaseClass::writeLock);
	BaseClass::hashMap_[k].push_back(v);
}

template <class KeyT, class ValueT>
ValueT MultiValueHashIndex<KeyT, ValueT>::getMiddle(const KeyT &k) {
	std::lock_guard<std::mutex> lock(BaseClass::writeLock);
	std::vector<ValueT> valVector = BaseClass::hashMap_[k];
	return valVector.at(valVector.size() / 2);
}


#endif /* SRC_INDEX_HASH_MULTIVALUEHASHINDEX_HPP_ */
