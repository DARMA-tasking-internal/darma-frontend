

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include "mpi.h"
#include <vector>



/* 
	Solve 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		d^2 u(x)/dx^2 = g(x)

	over (0,1), with T(0)=100, T(1)=10

*/


void solveThomas(std::vector<double> & a, 
								 std::vector<double> & b, 
								 std::vector<double> & c, 
								 std::vector<double> & d, 
								 int n) 
{
    /*

		Algorithm taken from web.

    // n is the number of unknowns
    |b0 c0 0 ||x0| |d0|
    |a1 b1 c1||x1|=|d1|
    |0  a2 b2||x2| |d2|

		1st iteration: b0x0 + c0x1 = d0 -> x0 + (c0/b0)x1 = d0/b0 ->

        x0 + g0x1 = r0               where g0 = c0/b0        , r0 = d0/b0

    2nd iteration:     | a1x0 + b1x1   + c1x2 = d1
        from 1st it.: -| a1x0 + a1g0x1        = a1r0
                    -----------------------------
                          (b1 - a1g0)x1 + c1x2 = d1 - a1r0

        x1 + g1x2 = r1               where g1=c1/(b1 - a1g0) , r1 = (d1 - a1r0)/(b1 - a1g0)

    3rd iteration:      | a2x1 + b2x2   = d2
        from 2st it. : -| a2x1 + a2g1x2 = a2r2
                       -----------------------
                       (b2 - a2g1)x2 = d2 - a2r2
        x2 = r2                      where                     r2 = (d2 - a2r2)/(b2 - a2g1)
    Finally we have a triangular matrix:
    |1  g0 0 ||x0| |r0|
    |0  1  g1||x1|=|r1|
    |0  0  1 ||x2| |r2|

    Condition: ||bi|| > ||ai|| + ||ci||

    in this version the c matrix reused instead of g
    and             the d matrix reused instead of r and x matrices to report results
    Written by Keivan Moradi, 2014
		*/

    n--; // since we start from x0 (not x1)
    c[0] /= b[0];
    d[0] /= b[0];

    for (int i = 1; i < n; i++) 
    {
				c[i] /= b[i] - a[i]*c[i-1];
        d[i] = (d[i] - a[i]*d[i-1]) / (b[i] - a[i]*c[i-1]);
    }

    d[n] = (d[n] - a[n]*d[n-1]) / (b[n] - a[n]*c[n-1]);

    for (int i = n; i-- > 0;)
    {
        d[i] -= c[i]*d[i+1];
    }
}



double rhs(double x)
{
	return 2.0 * exp(x) * cos(x);
}


double trueSolution(double x)
{
	return exp(x) * sin(x);
}



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

		d[i] = rhs(x) * dx*dx;

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

 	return 0;
}

