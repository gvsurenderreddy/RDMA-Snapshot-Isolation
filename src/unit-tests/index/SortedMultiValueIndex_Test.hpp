/*
 * SortedMultiValueIndex_Test.hpp
 *
 *  Created on: Apr 18, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_INDEX_SORTEDMULTIVALUEINDEX_TEST_HPP_
#define SRC_UNIT_TESTS_INDEX_SORTEDMULTIVALUEINDEX_TEST_HPP_

#include "../TestBase.hpp"
#include "../../index/hash/HashIndex.hpp"
#include <functional>	// std::function
#include <vector>

class SortedMultiValueIndex_Test : public TestBase {
private:
	static std::vector<std::function<void()>> functionList_;
public:
	static std::vector<std::function<void()>>& getFunctionList();
	static void test_general();
	static void test_manual_class();
};


#endif /* SRC_UNIT_TESTS_INDEX_SORTEDMULTIVALUEINDEX_TEST_HPP_ */
