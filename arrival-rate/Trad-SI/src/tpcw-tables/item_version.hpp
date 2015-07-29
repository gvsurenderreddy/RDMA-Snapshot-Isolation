/*
 *	item_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: erfanz
 */

#ifndef ITEM_VERSION_H_
#define ITEM_VERSION_H_

#include "item.hpp"

class ItemVersion {
public:
	int write_timestamp;
	Item item;
};

#endif // ITEM_VERSION_H_