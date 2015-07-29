#include <iostream>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <sched.h>	// for sched_setaffinity() and CPU_SET() and CPU_ZERO()
//#include "cpucounters.h"	// Intell PCM monitoring tool



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

int pin_to_CPU (int CPU_num){
	cpu_set_t  mask;
	CPU_ZERO(&mask);			// Clears set, so that it contains no CPUs.
	CPU_SET(CPU_num, &mask);	// Add CPU 'cpu_num' to set. 
	sched_setaffinity(0, sizeof(mask), &mask);	// 0 for the first parameter means the current process
	std::cout << "Current Process is pinned to core #" << CPU_num << std::endl;
	return 0;
}


int main() {
	pin_to_CPU (1);
	
	// PCM * m = PCM::getInstance();
	// PCM::ErrorCode returnResult = m->program();
	// if (returnResult != PCM::Success){
	// 	std::cerr << "Intel's PCM couldn't start" << std::endl;
	// 	std::cerr << "(Error code: " << returnResult << ")" << std::endl;
	//
	// 	exit(1);
	// }
	
	MyStruct *myS = new MyStruct[ARRAY_SIZE];
	unsigned long long cpu_checkpoint_start, cpu_checkpoint_finish;
	
	// **************************************
	// Initializing the array of structs, each element pointing to the next 
	for (int i=0; i < ARRAY_SIZE - 1; i++){
		myS[i].n = &myS[i + 1];
		for (int j = 0; j < NPAD; j++)
			myS[i].pad[j] = (long int) i;
	}
	myS[ARRAY_SIZE - 1].n = NULL;
	for (int j = 0; j < NPAD; j++)
		myS[ARRAY_SIZE - 1].pad[j] = (long int) (ARRAY_SIZE - 1);
	
	long int sum = 0;
	
	// Filling the cache
	MyStruct *current = &myS[0];
	while ((current = current->n) != NULL)
		sum += current->pad[0];
	
	// **************************************
	// Sequential access experiment
	current = &myS[0];
	
	sum = 0;
	
	// For CPU usage in terms of clocks (ticks)
	cpu_checkpoint_start = rdtscp();
	//SystemCounterState before_sstate = getSystemCounterState();
	
	while ((current = current->n) != NULL) {
		sum += current->pad[0];
	}
	
	// SystemCounterState after_sstate = getSystemCounterState();
// 	std::cout << "Instructions per clock:" << getIPC(before_sstate,after_sstate)
// 		<< "L3 cache hit ratio:" << getL3CacheHitRatio(before_sstate,after_sstate)
// 			<< "Bytes read:" << getBytesReadFromMC(before_sstate,after_sstate) << std::endl;
//
	
	cpu_checkpoint_finish = rdtscp();
	double average_cpu_clocks = (double)(cpu_checkpoint_finish - cpu_checkpoint_start) / ARRAY_SIZE;
	
	std::cout << "Avg CPU Clocks:	" << average_cpu_clocks << std::endl;
	std::cout << "Sum:	" << sum << std::endl;
	
	return 0;
}