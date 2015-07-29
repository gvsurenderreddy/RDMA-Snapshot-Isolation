/*
 *	utils.hpp
 *
 *	Created on: 26.Jan.2015
 *	Author: erfanz
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "../../config.hpp"
#include "../tpcw-tables/item_version.hpp"
#include <stdint.h>
#include <byteswap.h>

//#ifndef LOG_NAME
//#define LOG_NAME "MUST BE REDEFINED BY ALL FILES"
//#endif


#if(DEBUG_ENABLED)
# define DEBUG_COUT(x) do { std::cout << x << std::endl; } while( false )
# define DEBUG_CERR(x) do { std::cerr << x << std::endl; } while( false )
#else
# define DEBUG_COUT(x) do {} while (false)
# define DEBUG_CERR(x) do {} while (false)
#endif

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero).");  } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

#if defined(__i386__)
static __inline__ unsigned long long rdtscp(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)
static __inline__ unsigned long long rdtscp(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif




#if __BYTE_ORDER == __LITTLE_ENDIAN

static inline uint64_t hton64 (uint64_t x)
{
	bswap_64 (x);
	return x;
}
static inline uint32_t hton32 (uint32_t x)
{
	bswap_32 (x);
	return x;
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

#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif


/******************************************************************************
* Function: pin_to_CPU
*
* Input
* Core number
*
* Returns
* 0 on success
*
* Description
* Pins the current process to the specified core
******************************************************************************/
int pin_to_CPU (int CPU_num);

/******************************************************************************
* Function: generate_random_seed
*
* Input
* None
*
* Returns
* a random number, which can be used an input to srand()
*
* Description
* Generates a random seed to be used for srand() function
******************************************************************************/
unsigned long generate_random_seed();


int sync_it (int sock, char *local_buffer, int xfer_size);


int sock_write(int sock, char *buffer, int xfer_size);
int sock_read(int sock, char *buffer, int xfer_size);

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
******************************************************************************/
int sock_sync_data (int sock, int xfer_size, char *local_data, char *remote_data);



int open_socket();



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
int sock_connect (std::string servername, int port);


/******************************************************************************
* Function: establish_tcp_connection
*
* Input
* - remote_ip:		ip of the remote machine
* - remote_port:	port of the remote machine
*
* Output
* - sockfd is filled with the socket description of the connection
*	
*
* Returns
* 0 on success, -1 failure
*
* Description
* establishes a TCP connection to server: "remote_ip" and port: "remote_port"
* and puts the socket in "*sockfd"
******************************************************************************/
int establish_tcp_connection(std::string remote_ip, int remote_port, int *sockfd);

int server_socket_setup(int *server_sockfd, int backlog);

void die(const char *reason);

int load_tables_from_files(ItemVersion* items_region);


#endif /* UTILS_H_ */

