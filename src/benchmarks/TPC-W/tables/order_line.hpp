/*
 *	order_line.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ORDER_LINE_HPP_
#define ORDER_LINE_HPP_

namespace TPCW{
class OrderLine {
public:
	int OL_ID;
	int OL_O_ID;
	int OL_I_ID;
	int OL_QTY;
	double OL_DISCOUNT;
	char OL_COMMENTS [110];
};
}	// namespace TPCW

#endif /* ORDER_LINE_HPP */
