/*
 *	orders.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: erfanz
 */

#include <iostream>
using namespace std;


class Orders {
public:
	int O_ID;
	//int O_I_ID;	// I added this myself
	int O_C_ID;
	char O_DATE[10];
	double O_SUB_TOTAL;
	double O_TAX;
	double O_TOTAL;
	char O_SHIP_TYPE[10];
	char O_SHIP_DATE[19];
	int O_BILL_ADDR_ID;
	int O_SHIP_ADDR_ID;
	char O_STATUS[15];
};