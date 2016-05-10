/*
 * zipf.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

#ifndef UTIL_ZIPF_HPP_
#define UTIL_ZIPF_HPP_

#include <vector>

//===========================================================================
//=  Program to generate Zipf (power law) distributed random variables      =
//===========================================================================
class ZipfDistribution{
private:
    double alpha_;
    int n_;
	double c_;             // Normalization constant
	std::vector<double> sum_probs;

public:
	ZipfDistribution(double alpha, int n);
	int	getSample();  // Returns a Zipf random variable, in the range of [1 and n]
};

#endif /* UTIL_ZIPF_HPP_ */
