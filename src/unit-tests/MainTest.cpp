/*
 * MainTest.cpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#include "TestBase.hpp"

#include <functional>	// std::function
#include <vector>

//#include "tpcw-tests/Item_Test.hpp"
#include "index/HashIndex_Test.hpp"
#include "index/MultiValueHashIndex_Test.hpp"
#include "TPCC/TPCCIndices_Test.hpp"
#include "basic-types/Timestamp_Test.hpp"


template <typename T>
void appendVector(std::vector<T> &mainVec, const std::vector<T> &toBeAppendedVec) {
	mainVec.insert(mainVec.end(), toBeAppendedVec.begin(), toBeAppendedVec.end());
}

int main() {
	std::vector<std::function<void()>> allTestFunctions;
	appendVector(allTestFunctions, HashIndex_Test::getFunctionList());
	appendVector(allTestFunctions, MultiValueHashIndex_Test::getFunctionList());
	appendVector(allTestFunctions, TPCCIndices_Test::getFunctionList());
	appendVector(allTestFunctions, Timestamp_Test::getFunctionList());

	for (std::size_t i = 0; i < allTestFunctions.size(); i++){
		allTestFunctions.at(i)();
	}

	std::cout << std::endl << "All tests passed!" << std::endl;
	return 0;
}


