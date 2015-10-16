/*
 *	order_line_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ORDER_LINE_VERSION_HPP_
#define ORDER_LINE_VERSION_HPP_

#include "order_line.hpp"

class OrderLineVersion {
public:
	int write_timestamp;
	OrderLine order_line;
};

#endif /* ORDER_LINE_VERSION_HPP */
