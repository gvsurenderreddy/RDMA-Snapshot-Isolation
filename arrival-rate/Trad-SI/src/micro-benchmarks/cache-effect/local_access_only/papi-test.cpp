#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include "papi.h"

#define MX 1024
#define NITER 20
#define MEGA 1000000
#define TOT_FLOPS (2*MX*MX*NITER)

static const int length = 2;


double *ad[MX];

/* Get actual CPU time in seconds */
float gettime() 
{
	return((float)PAPI_get_virt_usec()*1000000.0);
}
int main () 
{
	float t0, t1;
	int iter, i, j;
	int events[length] = {PAPI_L1_DCM, PAPI_LST_INS}, ret;
	long long values[length];

	if (PAPI_num_counters() < length) {
		fprintf(stderr, "No hardware counters here, or PAPI not supported.\n");
		exit(1);
	}
	t0 = gettime();
	if ((ret = PAPI_start_counters(events, length)) != PAPI_OK) {
		fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(ret));
		exit(1);
	}

	if ((ret = PAPI_read_counters(values, length)) != PAPI_OK) {
		fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(ret));
		exit(1);
	}
	t1 = gettime();

	printf("Total software flops = %f\n",(float)TOT_FLOPS);
	printf("Total hardware flops = %lld\n",(float)values[1]);
	printf("MFlop/s = %f\n", (float)(TOT_FLOPS/MEGA)/(t1-t0));
	printf("L2 data cache misses is %lld\n", values[0]);
}