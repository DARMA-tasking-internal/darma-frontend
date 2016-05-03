#ifndef EXAMPLES_HEAT_1D_COMMON_H_
#define EXAMPLES_HEAT_1D_COMMON_H_

constexpr int n_iter  = 2500;  		// num of iterations in time
constexpr double deltaT = 0.05; 	// time step 
constexpr double alpha  = 0.0075; // diffusivity

constexpr int nx = 16;        		// total number of grid points  
constexpr double x_min = 0.0;     // domain start x 
constexpr double x_max = 1.0;     // domain end x
constexpr double deltaX = (x_max-x_min)/( (double) (nx-1) );  // cell spacing
constexpr double cfl = alpha * deltaT / (deltaX * deltaX );   // cfl condition
static_assert( cfl < 0.5, "cfl not small enough");  
// alpha * DT/ DX^2
constexpr double alphadtOvdxSq = (alpha * deltaT) / (deltaX * deltaX ); 

constexpr double Tl = 100.0;     // left BC for temperature
constexpr double Tr = 10.0;      // right BC for temperature

// steady state solution
double steadySolution(double x)
{
	const double a = (Tl-Tr)/(x_min-x_max);
	const double b = Tl - a * x_min;
	return a*x + b;
}

#endif /* EXAMPLES_HEAT_1D_COMMON_H_ */
