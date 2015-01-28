#include <iostream>
#include "orders.cpp"
using namespace std;

class OrdersVersion {
public:
	int write_timestamp;
	Orders orders;
};