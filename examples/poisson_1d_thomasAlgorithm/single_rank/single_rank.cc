

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include <vector>
#include <iomanip>

#include "../common_poisson1d.h"
#include <darma.h>



/* 
	Solve 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		d^2 u(x)/dx^2 = g(x)

	over (0,1), analytical solution is exp(x)*sin(x)

*/
	

////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
	using namespace darma_runtime;

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

	constexpr int nx = 51; 
	constexpr int nInn = nx-2;
	constexpr double xL = 0.0;
	constexpr double xR = 1.0;
	constexpr double dx = 1.0/static_cast<double>(nx-1);

	// handles with write privileges to vectors of doubles
	auto subD = initial_access<std::vector<double>>("a",me); // subdiagonal 
	auto diag = initial_access<std::vector<double>>("b",me); // diagonal 
	auto supD = initial_access<std::vector<double>>("c",me); // superdiagonal
	auto rhs  = initial_access<std::vector<double>>("d",me); // rhs and solution

	// initial condition
 	create_work([=]
 	{
 		subD.emplace_value();	// this will go away eventually
 		diag.emplace_value();
 		supD.emplace_value();
 		rhs.emplace_value();
 		subD->resize(nInn);
 		diag->resize(nInn,0.0);
 		supD->resize(nInn,0.0);
 		rhs->resize(nInn,0.0);
 		double * pt1 = subD->data();
 		double * pt2 = diag->data();
 		double * pt3 = supD->data();
 		double * pt4 = rhs->data();

		double x = dx;
		for (int i = 0; i < nInn; ++i)
		{
			pt2[i] = -2;

			if (i>0)
				pt1[i] = 1.0;
			if (i<nInn-1)
				pt3[i] = 1.0;

			pt4[i] = rhsEval(x) * dx*dx;

			if (i==1)
				pt4[i] -= trueSolution(xL);
			if (i==nInn-1)
				pt4[i] -= trueSolution(xR);

			x += dx;
		}

	});

 	// solve tridiagonal system
 	create_work([=]
 	{
 		int n=nInn;
 		std::cout << n << std::endl;
 		double * pta = subD->data();
 		double * ptb = diag->data();
 		double * ptc = supD->data();
 		double * ptd = rhs->data();

 		solveThomas(pta, ptb, ptc, ptd, n);
 	});


 	// check solution error 
 	create_work([=]
 	{
 		double * ptd = rhs->data();

		double error = 0.0;
		double x = dx;
		for (int i = 0; i < (int) rhs->size(); ++i)
		{
			error += std::abs( trueSolution(x) - ptd[i] );
			std::cout << " true = " << trueSolution(x) << " approx = " << ptd[i] << std::endl;
			x += dx;
		}
		std::cout << " L1 error = " << error << std::endl;
		assert( error < 1e-2 );
 	});

  darma_finalize();
  return 0;
}

