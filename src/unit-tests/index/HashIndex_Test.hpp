/*
 * hashIndexTest.hpp
 *
 *  Created on: Mar 30, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_INDEX_HASHINDEX_TEST_HPP_
#define SRC_UNIT_TESTS_INDEX_HASHINDEX_TEST_HPP_

#include "../TestBase.hpp"
#include "../../index/hash/HashIndex.hpp"
#include <functional>	// std::function
#include <vector>

class HashIndex_Test : public TestBase {
private:
	static std::vector<std::function<void()>> functionList_;
public:
	static std::vector<std::function<void()>>& getFunctionList();
	static void test_get_existing_key();
	static void test_get_nonexisting_key();
	static void test_get_deleted_key();
	static void test_put_nonexisting_key();
	static void test_put_existing_key();
	static void test_put_deleted_key();
};

#endif /* SRC_UNIT_TESTS_INDEX_HASHINDEX_TEST_HPP_ */
