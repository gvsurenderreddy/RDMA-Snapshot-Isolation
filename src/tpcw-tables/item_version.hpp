#include <iostream>
#include "item.cpp"
using namespace std;

class ItemVersion {
public:
	int write_timestamp;
	Item item;
};