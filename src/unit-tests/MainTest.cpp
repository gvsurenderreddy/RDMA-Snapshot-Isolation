/*
 * MainTest.cpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#include "TestBase.hpp"

#include <functional>	// std::function
#include <vector>

#include "tpcw-tests/Item_Test.hpp"

template <typename T>
void appendVector(std::vector<T> &mainVec, const std::vector<T> &toBeAppendedVec) {
	mainVec.insert(mainVec.end(), toBeAppendedVec.begin(), toBeAppendedVec.end());
}

int main() {
	std::vector<std::function<void(TestBase&)>> allTestFunctions;
//	appendVector(allTestFunctions, PointerTest::getFunctionList());
//	appendVector(allTestFunctions, KeyValueTest::getFunctionList());
//	appendVector(allTestFunctions, DependencyTest::getFunctionList());
//	appendVector(allTestFunctions, LogEntryTest::getFunctionList());
//	appendVector(allTestFunctions, LocalRegionContextTest::getFunctionList());
	//appendVector(allTestFunctions, SingleTreadedExecutionTest::getFunctionList());
	//appendVector(allTestFunctions, CoordinatorTest::getFunctionList());
	appendVector(allTestFunctions, Item_Test::getFunctionList());

	Item_Test itemTest;

	for (std::size_t i = 0; i < allTestFunctions.size(); i++){
		allTestFunctions.at(i)(itemTest);
	}

	std::cout << std::endl << "All tests passed!" << std::endl;
	return 0;
}


