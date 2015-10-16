/*
 *	item_version.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ITEM_VERSION_H_
#define ITEM_VERSION_H_

#include "item.hpp"
#include "../auxilary/timestamp.hpp"

class ItemVersion {
public:
	Timestamp	write_timestamp;
	Item		item;
};

#endif // ITEM_VERSION_H_
