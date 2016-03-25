/*
 * AbstractTable.hpp
 *
 *  Created on: Mar 22, 2016
 *      Author: erfanz
 */

#ifndef SRC_BENCHMARKS_TPC_C_TABLES_BASEVERSION_HPP_
#define SRC_BENCHMARKS_TPC_C_TABLES_BASEVERSION_HPP_


class BaseVersion {
public:
	virtual size_t getOffsetOfContent() const = 0;
	virtual size_t getOffsetOfTimestamp() const = 0;
	virtual size_t getOffsetInTable() const = 0;
	virtual ~BaseVersion(){}
};


#endif /* SRC_BENCHMARKS_TPC_C_TABLES_BASEVERSION_HPP_ */
