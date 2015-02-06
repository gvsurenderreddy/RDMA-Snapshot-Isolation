#include <iostream>
#include "orders.hpp"
using namespace std;

class OrdersVersion {
public:
	int write_timestamp;
	Orders orders;
};