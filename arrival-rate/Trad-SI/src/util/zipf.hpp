#ifndef ZIPF_H_
#define ZIPF_H_

int		zipf_initialize(double alpha, int n);
int		zipf_get_sample(double alpha, int n);  // Returns a Zipf random variable
double	rand_val();         // Jain's RNG

#endif /* ZIPF_H_ */