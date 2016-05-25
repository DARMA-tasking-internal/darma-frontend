#ifndef EXAMPLES_POISSON1D_CONSTS_H_
#define EXAMPLES_POISSON1D_CONSTS_H_

constexpr int nx = 51; 			// grid points 
constexpr int nInn = nx-2;	// inner grid points 
constexpr double xL = 0.0;	// left boundary 
constexpr double xR = 1.0;	// right boundary 
constexpr double dx = 1.0/(double)(nx-1);		// grid spacing

#endif /* EXAMPLES_POISSON1D_CONSTS_H_ */
