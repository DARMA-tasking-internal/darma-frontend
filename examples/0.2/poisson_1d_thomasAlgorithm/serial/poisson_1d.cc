

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include "mpi.h"
#include <vector>

#include "../common_poisson1d.h"


/* 
	Solve 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		d^2 u(x)/dx^2 = g(x)

	over (0,1), analytical solution is exp(x)*sin(x)

*/

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
	constexpr int nx = 51; 
	constexpr int nInn = nx-2;
	constexpr double xL = 0.0;
	constexpr double xR = 1.0;
	constexpr double dx = 1.0/static_cast<double>(nx-1);

	std::vector<double> a(nInn,0.0); // subdiagonal 
	std::vector<double> b(nInn,0.0); // diagonal 
	std::vector<double> c(nInn,0.0); // superdiagonal
	std::vector<double> d(nInn,0.0); // rhs and solution

	double x = dx;
	for (int i = 0; i < nInn; ++i)
	{
		b[i] = -2;

		if (i>0)
			a[i] = 1.0;
		if (i<nInn-1)
			c[i] = 1.0;

		d[i] = rhsEval(x) * dx*dx;

		if (i==1)
			d[i] -= trueSolution(xL);
		if (i==nInn-1)
			d[i] -= trueSolution(xR);

		x += dx;
	}

	solveThomas(a, b, c, d, nInn); 

	double error = 0.0;
	x = dx;
	for (auto & it : d)
	{
		error += std::abs( trueSolution(x) - it );
		// std::cout << " true = " << trueSolution(x) << " approx = " << it << std::endl;
		x += dx;
	}
	std::cout << " L1 error = " << error << std::endl;
	assert( error < 1e-2 );

	std::cout << " mid domain value = " << d[(nInn-1)/2] << std::endl;

 	return 0;
}

