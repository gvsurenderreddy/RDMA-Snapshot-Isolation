/*
 *	order_line_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ORDER_LINE_VERSION_HPP_
#define ORDER_LINE_VERSION_HPP_

#include "order_line.hpp"

namespace TPCW{
class OrderLineVersion {
public:
	int write_timestamp;
	OrderLine order_line;
};
}	// namespace TPCW

#endif /* ORDER_LINE_VERSION_HPP */
