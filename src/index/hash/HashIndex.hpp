/*
 * HashIndex.hpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#ifndef SRC_INDEX_HASH_HASHINDEX_HPP_
#define SRC_INDEX_HASH_HASHINDEX_HPP_

#include <unordered_map>
#include <iostream>
#include <cstring>      // std::memcpy

template <class KeyT, class ValueT>
class HashIndex {

protected:
   std::unordered_map<KeyT, ValueT> hashMap_;
public:
	HashIndex();
	virtual ~HashIndex();

    void put(const KeyT &k, const ValueT &v);
    ValueT get(const KeyT &k) const;
    void erase(const KeyT &k);
    void printAll() const;
};


/* Note that template classes must be implemented in the header file, since the compiler
 * needs to have access to the implementation of the methods when instantiating the template class at compile time.
 * For more information, please refer to:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 */


template <class KeyT, class ValueT>
HashIndex<KeyT, ValueT>::HashIndex() {
	// TODO Auto-generated constructor stub
}

template <class KeyT, class ValueT>
HashIndex<KeyT, ValueT>::~HashIndex() {
	// TODO Auto-generated destructor stub
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::put(const KeyT &k, const ValueT &v) {
	std::memcpy(&hashMap_[k], &v, sizeof(ValueT));
}

template <class KeyT, class ValueT>
ValueT HashIndex<KeyT, ValueT>::get(const KeyT &k) const{
	return hashMap_.at(k);
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::erase(const KeyT &k){
	hashMap_.erase(k);
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::printAll() const {
	for (auto& x: hashMap_)
		std::cout << x.first << ": " << x.second << std::endl;
}


#endif /* SRC_INDEX_HASH_HASHINDEX_HPP_ */
