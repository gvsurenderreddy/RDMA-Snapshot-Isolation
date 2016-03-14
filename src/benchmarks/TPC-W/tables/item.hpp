/*
 *	item.hpp
 *
 *	Created on: 22.Jan.2015
 *	Author: Erfan Zamanian
 */

#ifndef ITEM_HPP_
#define ITEM_HPP_

namespace TPCW {
class Item {
public:
	int I_ID;
	char I_TITLE[60];
	int I_A_ID;
	char I_PUB_DATE[11];
	char I_PUBLISHER[60];
	char I_SUBJECT[60];
	char I_DESC[500];
	int I_RELATED1;
	int I_RELATED2;
	int I_RELATED3;
	int I_RELATED4;
	int I_RELATED5;
	char I_THUMBNAIL[40];
	char I_IMAGE[40];
	double I_SRP;
	double I_COST;
	char I_AVAIL[10];
	int I_STOCK;
	char I_ISBN[13];
	int I_PAGE;
	char I_BACKING[15];
	char I_DIMENSION[25];
};
}	// namespace TPCW

#endif /* ITEM_HPP */
