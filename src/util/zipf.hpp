/*
 * zipf.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: Erfan Zamanian
 */

#ifndef UTIL_ZIPF_HPP_
#define UTIL_ZIPF_HPP_

int		zipf_initialize(double alpha, int n);
int		zipf_get_sample(double alpha, int n);  // Returns a Zipf random variable
double	rand_val();

#endif /* UTIL_ZIPF_HPP_ */
