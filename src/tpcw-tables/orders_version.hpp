/*
 *	orders_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ORDERS_VERSION_HPP_
#define ORDERS_VERSION_HPP_

#include "orders.hpp"

class OrdersVersion {
public:
	int write_timestamp;
	Orders orders;
};

#endif /* ORDERS_VERSION_HPP_ */
