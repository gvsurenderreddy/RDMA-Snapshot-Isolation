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
#include <cstring>      // for std::memcpy
#include <mutex>		// for std::lock_guard

template <class KeyT, class ValueT>
class HashIndex {

protected:
   std::unordered_map<KeyT, ValueT> hashMap_;
   std::mutex writeLock;

public:
	HashIndex();
	virtual ~HashIndex();

    void put(const KeyT &k, const ValueT &v);
    ValueT get(const KeyT &k);
    bool hasKey(const KeyT &k);
    void erase(const KeyT &k);
    void clear();
    size_t size();
    void printAll();

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
	std::lock_guard<std::mutex> lock(writeLock);
	//std::memcpy(&hashMap_[k], &v, sizeof(ValueT));
	hashMap_[k] = v;
}

template <class KeyT, class ValueT>
ValueT HashIndex<KeyT, ValueT>::get(const KeyT &k){
	std::lock_guard<std::mutex> lock(writeLock);
	return hashMap_.at(k);
}

template <class KeyT, class ValueT>
bool HashIndex<KeyT, ValueT>::hasKey(const KeyT &k){
	std::lock_guard<std::mutex> lock(writeLock);
	return hashMap_.find(k) != hashMap_.end();
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::erase(const KeyT &k){
	std::lock_guard<std::mutex> lock(writeLock);
	hashMap_.erase(k);
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::clear(){
	std::lock_guard<std::mutex> lock(writeLock);
	hashMap_.clear();
}

template <class KeyT, class ValueT>
size_t HashIndex<KeyT, ValueT>::size(){
	std::lock_guard<std::mutex> lock(writeLock);
	return hashMap_.size();
}

template <class KeyT, class ValueT>
void HashIndex<KeyT, ValueT>::printAll() {
	std::lock_guard<std::mutex> lock(writeLock);
	for (auto& x: hashMap_)
		std::cout << x.first << ": " << x.second << std::endl;
}


#endif /* SRC_INDEX_HASH_HASHINDEX_HPP_ */
