/*
 * TestBase.hpp
 *
 *  Created on: Feb 21, 2016
 *      Author: erfanz
 */

#ifndef SRC_UNIT_TESTS_TESTBASE_HPP_
#define SRC_UNIT_TESTS_TESTBASE_HPP_


#include <string>
#include <iostream>

class TestBase {
protected:
	static void printMessage(std::string className, std::string functionName) {
		std::cout << "[Unit Testing] " << className << "::" << functionName << "() ... " << std::endl;
	}
};

#endif /* SRC_UNIT_TESTS_TESTBASE_HPP_ */
