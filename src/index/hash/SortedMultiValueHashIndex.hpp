/*
 * SortedMultiValueHashIndex.hpp
 *
 *  Created on: Apr 18, 2016
 *      Author: erfanz
 */

#ifndef SRC_INDEX_HASH_SORTEDMULTIVALUEHASHINDEX_HPP_
#define SRC_INDEX_HASH_SORTEDMULTIVALUEHASHINDEX_HPP_


#include "HashIndex.hpp"
#include <queue>

template <class KeyT, class ValueT>
class SortedMultiValueHashIndex : public HashIndex<KeyT, std::priority_queue<ValueT, std::vector<ValueT>, std::greater<ValueT> > > {

typedef HashIndex<KeyT, std::priority_queue<ValueT, std::vector<ValueT>, std::greater<ValueT> > > BaseClass;

public:
SortedMultiValueHashIndex();
	virtual ~SortedMultiValueHashIndex();

	void push(const KeyT &k, const ValueT &v);
	const ValueT& top(const KeyT &k) const;
	void pop(const KeyT &k);
};


/* Note that template classes must be implemented in the header file, since the compiler
 * needs to have access to the implementation of the methods when instantiating the template class at compile time.
 * For more information, please refer to:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 */


template <class KeyT, class ValueT>
SortedMultiValueHashIndex<KeyT, ValueT>::SortedMultiValueHashIndex() {
	// TODO Auto-generated constructor stub

}
template <class KeyT, class ValueT>
SortedMultiValueHashIndex<KeyT, ValueT>::~SortedMultiValueHashIndex() {
	// TODO Auto-generated destructor stub
}

template <class KeyT, class ValueT>
void SortedMultiValueHashIndex<KeyT, ValueT>::push(const KeyT &k, const ValueT &v) {
	BaseClass::hashMap_[k].push(v);
}

template <class KeyT, class ValueT>
const ValueT& SortedMultiValueHashIndex<KeyT, ValueT>::top(const KeyT &k) const {
	const std::priority_queue<ValueT, std::vector<ValueT>, std::greater<ValueT> > &queue = BaseClass::hashMap_.at(k);
	if (!queue.empty())
		queue.top();
	else throw std::out_of_range("Empty list");
}

template <class KeyT, class ValueT>
void SortedMultiValueHashIndex<KeyT, ValueT>::pop(const KeyT &k) {
	BaseClass::hashMap_[k].pop();
}



#endif /* SRC_INDEX_HASH_SORTEDMULTIVALUEHASHINDEX_HPP_ */
