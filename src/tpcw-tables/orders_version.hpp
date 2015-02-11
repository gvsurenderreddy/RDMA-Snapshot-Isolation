/*
 *	orders_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: erfanz
 */

#include <iostream>
#include "orders.hpp"
using namespace std;

class OrdersVersion {
public:
	int write_timestamp;
	Orders orders;
};