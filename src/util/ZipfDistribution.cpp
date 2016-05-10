/*
 * zipf.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

#include "ZipfDistribution.hpp"
#include "utils.hpp"
#include <assert.h>             // Needed for assert() macro
#include <stdio.h>              // Needed for printf()
#include <stdlib.h>             // Needed for exit(), rand()
#include <math.h>               // Needed for pow()
//#include <time.h>				// timespec



//int main(int argc, char* argv[]) {
//	struct timespec a, b;
//
//	double alpha = 3;
//	int n = 1000;
//
//
//	int array[n];
//	for (int i = 0; i < n; i ++)
//		array[i] = 0;
//
//	double time = 0;
//	ZipfDistribution z = ZipfDistribution(alpha, n);
//	int numbersCnt = std::stoi(std::string(argv[1]));
//	for (int i = 0; i < numbersCnt; i++){
//		//printf("%d\n", zipf_get_sample(alpha, n));
//		clock_gettime(CLOCK_REALTIME, &a);
//		int num = z.getSample();
//		clock_gettime(CLOCK_REALTIME, &b);
//
//		array[num - 1]++;
//		time += ( (double)( b.tv_sec - a.tv_sec ) * 1E9 + (double)( b.tv_nsec - a.tv_nsec ) ) / 1000;
//
//	}
//	std::cout << "Time (s): " << time / (1000 * 1000) << std::endl;
//
//
////	for (int i = 0; i < n; i ++) {
////		if (array[i] != 0)
////			std::cout << "# occurences of value " << i << ": " << array[i] << std::endl;
////	}
//}

ZipfDistribution::ZipfDistribution(double alpha, int n)
: alpha_(alpha),
  n_(n),
  sum_probs(n_){
	srand ((unsigned int)utils::generate_random_seed());		// initialize random seed

	// Compute normalization constant on first call only
	c_ = 0;
	for (int i=1; i <= n; i++)
		c_ = c_ + (1.0 / pow((double) i, alpha));
	c_ = 1.0 / c_;

	double sum = 0;
	for (int i=1; i<= n_; i++) {
		sum_probs[i-1] = sum + c_ / pow((double) i, alpha_);
		sum = sum_probs[i-1];
	}
}

int ZipfDistribution::getSample() {
	double z;                     // Uniform random number (0 < z < 1)
	int zipf_value;            // Computed exponential value to be returned

	// Pull a uniform random number (0 < z < 1)
	do {
		z = ((double) rand() / RAND_MAX) ;
	}
	while ((z == 0) || (z == 1));

	// Map z to the value
	// use binary search on sum_probs array instead of a sequential scan
	int lowInd = 0;
	int highInd = n_ - 1;
	int middle;
	while (highInd > lowInd){
		middle = (highInd + lowInd) / 2;
		if (sum_probs[middle] >= z)
			highInd = middle;
		else
			lowInd = middle + 1;
	}
	zipf_value = highInd + 1;

	// Assert that zipf_value is between 1 and N
	assert((zipf_value >=1) && (zipf_value <= n_));
	return(zipf_value);
}
