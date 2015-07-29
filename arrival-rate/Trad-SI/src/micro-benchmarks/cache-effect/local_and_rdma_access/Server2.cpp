#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <getopt.h>
#include <netdb.h>
#include <iostream>
#include <algorithm>    // std::shuffle
#include <vector>       // std::vector
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock



# define DEBUG_COUT(x) do { std::cout << x << std::endl; } while( false )
# define DEBUG_CERR(x) do { std::cerr << x << std::endl; } while( false )

void die(const char *reason)
{
	std::cerr << reason << std::endl;
	std::cerr << "Errno: " << strerror(errno) << std::endl;
	exit(EXIT_FAILURE);
}

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







static const int NPAD		= 31;
static const int ARRAY_SIZE	= 4194304;


struct MyStruct {
	struct MyStruct *n;
	long int pad[NPAD];
};
MyStruct *global_buffer;






void make_sequential_linked_list(struct MyStruct *myS){	
	for (int i=0; i < ARRAY_SIZE - 1; i++){
		myS[i].n = &myS[i + 1];
		for (int j = 0; j < NPAD; j++)
			myS[i].pad[j] = (long int) i;
	}
	myS[ARRAY_SIZE - 1].n = NULL;
	for (int j = 0; j < NPAD; j++)
		myS[ARRAY_SIZE - 1].pad[j] = (long int) (ARRAY_SIZE - 1);
}

int initialize_data_structures(){
	global_buffer = new MyStruct[ARRAY_SIZE];
	make_sequential_linked_list(global_buffer);
	// make_random_linked_list(global_buffer);
	return 0;
}

	

int start_benchmark(){
	// ************************************************	
	// Now, server performs a sequential scan
	
	// Filling the cache
	long int sum = 0;
	MyStruct *current = &global_buffer[0];
	while ((current = current->n) != NULL)
		sum += current->pad[0];
	
	DEBUG_COUT("[INFO] Cached initialized");
	
	current = &global_buffer[0];		
	
	// For CPU usage in terms of clocks (ticks)	
	sum  = 0;
	
	unsigned long long cpu_checkpoint_start = rdtscp();
	while (true) {
		current = current->n;
		if (current == NULL) break;
		// DEBUG_COUT("[INFO] Accessed sequentially for global_buffer[" << current->pad[0] << "]");
		
		sum += current->pad[0];
	}
	unsigned long long cpu_checkpoint_finish = rdtscp();
	
	double average_cpu_clocks = (double)(cpu_checkpoint_finish - cpu_checkpoint_start) / ARRAY_SIZE;
	std::cout << "Avg CPU Clocks:	" << average_cpu_clocks << std::endl;
	std::cout << "Sum:	" << sum << std::endl;
	// ************************************************
	
	return 0;
}


int start_server () {
	TEST_NZ(initialize_data_structures());	
	TEST_NZ(start_benchmark());
	// Server waits for the client to muck with its memory
	std::cout << "[Info] Server's ready to gracefully get destroyed" << std::endl;	
	return 0;
}

int main (int argc, char *argv[]) {
	start_server();
	return 0;
}