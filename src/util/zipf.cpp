/*
 * zipf.cpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

//==================================================== file = genzipf.c =====
//=  Program to generate Zipf (power law) distributed random variables      =
//===========================================================================
//=  Notes: 1) Writes to a user specified output file                       =
//=         2) Generates user specified number of values                    =
//=         3) Run times is same as an empirical distribution generator     =
//=         4) Implements p(i) = C/i^alpha for i = 1 to N where C is the    =
//=            normalization constant (i.e., sum of p(i) = 1).              =
//=-------------------------------------------------------------------------=
//= Example user input:                                                     =
//=                                                                         =
//=   ---------------------------------------- genzipf.c -----              =
//=   -     Program to generate Zipf random variables        -              =
//=   --------------------------------------------------------              =
//=   Output file name ===================================> output.dat      =
//=   Random number seed =================================> 1               =
//=   Alpha vlaue ========================================> 1.0             =
//=   N value ============================================> 1000            =
//=   Number of values to generate =======================> 5               =
//=   --------------------------------------------------------              =
//=-------------------------------------------------------------------------=
//= Example output file ("output.dat" for above):                           =
//=                                                                         =
//=   1                                                                     =
//=   1                                                                     =
//=   161                                                                   =
//=   17                                                                    =
//=   30                                                                    =
//=-------------------------------------------------------------------------=

#include "utils.hpp"
#include "zipf.hpp"
#include <assert.h>             // Needed for assert() macro
#include <stdio.h>              // Needed for printf()
#include <stdlib.h>             // Needed for exit(), rand()
#include <math.h>               // Needed for pow()

static long x;               // Random int value
static double c = 0;          // Normalization constant

// int main() {
// 	double alpha = 10;
// 	int n = 9;
// 	zipf_initialize(alpha, n);
// 	for (int i = 0; i < 100000; i++)
// 		printf("%d\n", zipf_get_sample(alpha, n));
// }

int zipf_initialize(double alpha, int n) {
	x = utils::generate_random_seed();

	// Compute normalization constant on first call only
	for (int i=1; i <= n; i++)
		c = c + (1.0 / pow((double) i, alpha));
	c = 1.0 / c;

	return 0;
}

//===========================================================================
//=  Function to generate Zipf (power law) distributed random variables     =
//=    - Input: alpha and N                                                 =
//=    - Output: Returns with Zipf distributed random variable              =
//===========================================================================
int zipf_get_sample(double alpha, int n) {
	double z;                     // Uniform random number (0 < z < 1)
	double sum_prob;              // Sum of probabilities
	int zipf_value;            // Computed exponential value to be returned

	// Pull a uniform random number (0 < z < 1)
	do {
		//z = rand_val();
		z = ((double) rand() / RAND_MAX) ;
	}
	while ((z == 0) || (z == 1));

	// Map z to the value
	sum_prob = 0;
	for (int i=1; i<=n; i++) {
		sum_prob = sum_prob + c / pow((double) i, alpha);
		if (sum_prob >= z) {
			zipf_value = i;
			break;
		}
	}
	// Assert that zipf_value is between 1 and N
	assert((zipf_value >=1) && (zipf_value <= n));
	return(zipf_value);
}
