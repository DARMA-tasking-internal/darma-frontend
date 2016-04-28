#ifndef EXAMPLES_POISSON1D_CONSTS_H_
#define EXAMPLES_POISSON1D_CONSTS_H_

constexpr int nx = 51; 
constexpr int nInn = nx-2;
constexpr double xL = 0.0;
constexpr double xR = 1.0;
constexpr double dx = 1.0/(double)(nx-1);

#endif /* EXAMPLES_POISSON1D_CONSTS_H_ */
