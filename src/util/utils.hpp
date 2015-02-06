/*
 *	utils.hpp
 *
 *	Created on: 26.01.2015
 *	Author: erfanz
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <byteswap.h>



#if __BYTE_ORDER == __LITTLE_ENDIAN
/*
static inline uint64_t htonll (uint64_t x)
{
	return bswap_64 (x);
}

static inline uint64_t ntohll (uint64_t x)
{
	return bswap_64 (x);
}
*/
static inline uint64_t hton64 (uint64_t x)
{
	bswap_64 (x);
}
static inline uint32_t hton32 (uint32_t x)
{
	bswap_32 (x);
}

#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t hton64 (uint64_t x)
{
	return x;
}
static inline uint32_t hton32 (uint32_t x)
{
	return x;
}


/*
static inline uint64_t htonll (uint64_t x)
{
	return x;
}

static inline uint64_t ntohll (uint64_t x)
{
	return x;
}
*/

#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif


#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
# define DEBUG_COUT(x) do { std::cout << x << std::endl; } while( false )
# define DEBUG_CERR(x) do { std::cerr << x << std::endl; } while( false )
#else
# define DEBUG_COUT(x) do {} while (false)
# define DEBUG_CERR(x) do {} while (false)
#endif

/******************************************************************************
* Function: sock_sync_data
*
* Input
* sock socket to transfer data on
* xfer_size size of data to transfer
* local_data pointer to data to be sent to remote
*
* Output
* remote_data pointer to buffer to receive remote data
*
* Returns
* 0 on success, negative error code on failure
*
* Description
* Sync data across a socket. The indicated local data will be sent to the
* remote. It will then wait for the remote to send its data back. It is
* assumed that the two sides are in sync and call this function in the proper
* order. Chaos will ensue if they are not. :)
*
* Also note this is a blocking function and will wait for the full data to be
* received from the remote.
*
******************************************************************************/
int sock_sync_data (int sock, int xfer_size, char *local_data, char *remote_data);


/******************************************************************************
* Function: sock_connect
*
* Input
* servername URL of server to connect to (NULL for server mode)
* port port of service
*
* Output
* none
*
* Returns
* socket (fd) on success, negative error code on failure
*
* Description
* Connect a socket. If servername is specified a client connection will be
* initiated to the indicated server and port. Otherwise listen on the
* indicated port for an incoming connection.
*
******************************************************************************/
int sock_connect (const char *servername, int port);

#endif /* UTILS_H_ */

