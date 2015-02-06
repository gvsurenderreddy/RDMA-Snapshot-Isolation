#include <iostream>
#include "item.hpp"
using namespace std;

class ItemVersion {
public:
	int write_timestamp;
	Item item;
};