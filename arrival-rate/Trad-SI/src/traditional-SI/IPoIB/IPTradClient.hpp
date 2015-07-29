/*
 *	IPTradClient.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: erfanz
 */

#ifndef IP_TRAD_CLIENT_H_
#define IP_TRAD_CLIENT_H_

#include "IPTradClientContext.hpp"
#include "../BaseTradClient.hpp"
#include "../../../config.hpp"
#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>


class IPTradClient : public BaseTradClient{
private:
	
	int start_transaction(IPTradClientContext &ctx);	

public:
	int start_client ();
};

#endif /* IP_TRAD_CLIENT_H_ */
