/*
 *	IPTradClient.hpp
 *
 *	Created on: 21.Feb.2015
 *	Author: Erfan Zamanian
 */

#ifndef IP_TRAD_CLIENT_H_
#define IP_TRAD_CLIENT_H_

#include "../../../config.hpp"
#include <byteswap.h>
#include <stdint.h>
#include <stdlib.h>
#include <infiniband/verbs.h>
#include "../../two-sided/BaseTradClient.hpp"
#include "../../two-sided/IPoIB/IPTradClientContext.hpp"


class IPTradClient : public BaseTradClient{
private:
	
	int start_transaction(IPTradClientContext &ctx);	

public:
	int start_client ();
};

#endif /* IP_TRAD_CLIENT_H_ */
