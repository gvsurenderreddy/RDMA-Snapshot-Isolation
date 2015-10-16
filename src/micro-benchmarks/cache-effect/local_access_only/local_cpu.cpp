#include <iostream>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <sched.h>		// for sched_setaffinity() and CPU_SET() and CPU_ZERO()
#include <algorithm>    // std::shuffle
#include <vector>       // std::vector
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <iterator> 	// for ostream_iterator
// #include "cpucounters.h"	// Intell PCM monitoring tool
#include "papi.h"




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
static const int ARRAY_SIZE	= 16;

struct MyStruct {
	struct MyStruct *n;
	long int pad[NPAD];
};

int pin_to_CPU (int CPU_num){
	cpu_set_t  mask;
	CPU_ZERO(&mask);			// Clears set, so that it contains no CPUs.
	CPU_SET(CPU_num, &mask);	// Add CPU 'cpu_num' to set. 
	sched_setaffinity(0, sizeof(mask), &mask);	// 0 for the first parameter means the current process
	std::cout << "Current Process is pinned to core #" << CPU_num << std::endl;
	return 0;
}


// *********************************************************************
// Initializing the array of structs, each element pointing to the next 
void make_sequential_linked_list(struct MyStruct *myS){	
	for (int i=0; i < ARRAY_SIZE - 1; i++){
		myS[i].n = &myS[i + 1];
		for (int j = 0; j < NPAD; j++)
			myS[i].pad[j] = (long int) i;
	}
	myS[ARRAY_SIZE - 1].n = &myS[0];
	for (int j = 0; j < NPAD; j++)
		myS[ARRAY_SIZE - 1].pad[j] = (long int) (ARRAY_SIZE - 1);
}

// *********************************************************************
// Initializing the array of structs, so each element pointing to a random element 
void make_random_linked_list(struct MyStruct *myS){
	std::vector<int> seq;
	for (int i = 1; i < ARRAY_SIZE; i++)
		seq.push_back(i);
	
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle (seq.begin(), seq.end(), std::default_random_engine(seed));
	
	//std::copy(sequence.begin(), sequence.end(), std::ostream_iterator<int>(std::cout, " "));
	
	// First, we link myS[0] to the first element of the shuffled sequence (this way, myS[0] is always the beginning of the list) 
	myS[0].n = &myS[seq[0]];
	for (int j = 0; j < NPAD; j++)
		myS[0].pad[j] = (long int) 0;
	
	for(unsigned int i = 0; i < seq.size() - 1; i++) {
		myS[seq[i]].n = &myS[seq[i + 1]];
		for (int j = 0; j < NPAD; j++)
			myS[seq[i]].pad[j] = (long int) seq[i];
	}
	
	myS[seq.back()].n = NULL;
	for (int j = 0; j < NPAD; j++)
		myS[seq.back()].pad[j] = (long int) seq.back();
}



int main() {
	int events[1] = {PAPI_L1_DCM}, ret;
	long long values[1];
	
	
	
	// pin_to_CPU (1);
	
	// PCM * m = PCM::getInstance();
	// PCM::ErrorCode returnResult = m->program(PCM::DEFAULT_EVENTS, NULL);
	// if (returnResult != PCM::Success){
	// 	std::cerr << "Intel's PCM couldn't start" << std::endl;
	// 	std::cerr << "(Error code: " << returnResult << ")" << std::endl;
	//
	// 	exit(1);
	// }
	
	if (PAPI_num_counters() < 1) {
	   fprintf(stderr, "No hardware counters here, or PAPI not supported.\n");
	   exit(1);
	}
	
	
	MyStruct *myS = new MyStruct[ARRAY_SIZE];
	// unsigned long long cpu_checkpoint_start, cpu_checkpoint_finish;
	
	//make_sequential_linked_list(myS);
	make_random_linked_list(myS);
	
	
	// Filling the cache
	
	// MyStruct *current = &myS[0];
	// for (int i = 0; i < 200000; i++){
	// 	current = &myS[0];
	// 	while ((current = current->n) != NULL)
	// 		current->pad[0] += 1;
	// }
	
	for (int i = 0; i < 20000; i++){
		int j = 0;
		while (j < ARRAY_SIZE){
			myS[j].pad[0] += 1;
			j++;
		}
	}
	
	// **************************************
	// Sequential access experiment
	// current = &myS[0];
	// sum = 0;

	// For CPU usage in terms of clocks (ticks)
	//cpu_checkpoint_start = rdtscp();
	
	// SystemCounterState before_sstate = getSystemCounterState();
	
	
	if ((ret = PAPI_start_counters(events, 1)) != PAPI_OK) {
	   fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
	   exit(1);
	}
	
	
	// for (int i = 0; i < 2000000; i++){
	// 	int j = 0;
	// 	while (j < ARRAY_SIZE){
	// 		myS[j].pad[0] += 1;
	// 		j++;
	// 	}
	// }
	
	if ((ret = PAPI_read_counters(values, 1)) != PAPI_OK) {
	  fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
	  exit(1);
	}
	
	//cpu_checkpoint_finish = rdtscp();
	// SystemCounterState after_sstate = getSystemCounterState();
	
	
	// prevent compiler to ignore the loop
	long int sum = 0;
	for (int i = 0; i < ARRAY_SIZE; i++){
		sum += myS[i].pad[0];
	}
	
	std::cout << "L1 " << values[0] << std::endl;
	// std::cout << "L2 " << values[1] << std::endl;
// 	std::cout << "L3 " << values[2] << std::endl;
//
	
	// std::cout << "Instructions per clock: " << getIPC(before_sstate,after_sstate)  << std::endl;
	// std::cout << "Cycles per operation: " << (double) getCycles(before_sstate,after_sstate) / ARRAY_SIZE << std::endl;
	// std::cout << "Bytes read:           " << getBytesReadFromMC(before_sstate,after_sstate) << std::endl;
	// std::cout << "L2 Cache Misses:      " << getL2CacheMisses(before_sstate,after_sstate) << std::endl;
	// std::cout << "L2 Cache Hits:        " << getL2CacheHits(before_sstate,after_sstate) << std::endl;
	// std::cout << "L2 cache hit ratio:   " << getL2CacheHitRatio(before_sstate, after_sstate) << std::endl;
	// std::cout << "L3 Cache Misses:      " << getL3CacheMisses(before_sstate,after_sstate) << std::endl;
	// std::cout << "L3 Cache Hits:        " << getL3CacheHits(before_sstate,after_sstate) << std::endl;
	// std::cout << "L3 cache hit ratio:   " << getL3CacheHitRatio(before_sstate,after_sstate) << std::endl;
	
	
	// double average_cpu_clocks = (double)(cpu_checkpoint_finish - cpu_checkpoint_start) / ARRAY_SIZE;
	
	// std::cout << "Avg CPU Clocks:	" << average_cpu_clocks << std::endl;
	std::cout << "Sum:	" << sum << std::endl;
	
	// m->cleanup();
	
	return 0;
}