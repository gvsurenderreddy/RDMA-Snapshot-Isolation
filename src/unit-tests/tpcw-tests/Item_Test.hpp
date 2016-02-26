/*
 * Item_Test.hpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_TPCW_TESTS_ITEM_TEST_HPP_
#define SRC_UNIT_TESTS_TPCW_TESTS_ITEM_TEST_HPP_

#include "../TestBase.hpp"
#include "../../benchmarks/TPC-C/tables/ItemTable.hpp"
#include <functional>	// std::function
#include <vector>


class Item_Test : public TestBase {
private:
	static std::vector<std::function<void(TestBase&)>> functionList_;
	TPCC::ItemTable *itemTable;
public:
	static std::vector<std::function<void(TestBase&)>>& getFunctionList();
	void test_constructor();
	Item_Test();
	virtual ~Item_Test();

};

#endif /* SRC_UNIT_TESTS_TPCW_TESTS_ITEM_TEST_HPP_ */
