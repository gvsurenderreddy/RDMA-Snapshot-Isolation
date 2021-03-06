/*
 *	BaseTradClient.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef BASE_TRAD_CLIENT_H_
#define BASE_TRAD_CLIENT_H_

#include "../../config.hpp"
#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../two-sided/TradMessage.hpp"



class BaseTradClient{
protected:
	static void fill_shopping_cart(struct Cart *cart, int sockfd);

public:
	virtual int start_client () = 0;
	
	static void usage (const char *argv0);
};

#endif /* BASE_TRAD_CLIENT_H_ */