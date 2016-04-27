

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

using namespace darma_runtime;



/* 
	Solve 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		d^2 u(x)/dx^2 = g(x)

	over (0,1), analytical solution is exp(x)*sin(x)

*/
	

void initialize(AccessHandle<std::vector<double>> & subD,
								AccessHandle<std::vector<double>> & diag, 
								AccessHandle<std::vector<double>> & supD,
								AccessHandle<std::vector<double>> & rhs)
{
	// initial condition
 	create_work([=]
 	{
    subD.emplace_value();
    diag.emplace_value();
    supD.emplace_value();
    rhs.emplace_value();
    subD->resize(nInn);
    diag->resize(nInn,0.0);
    supD->resize(nInn,0.0);
    rhs->resize(nInn,0.0);
    double * ptrD1 = subD->data();
    double * ptrD2 = diag->data();
    double * ptrD3 = supD->data();
    double * ptrD4 = rhs->data();

    double x = dx;
    for (int i = 0; i < nInn; ++i)
    {
      ptrD2[i] = -2;

      if (i>0)
        ptrD1[i] = 1.0;
      if (i<nInn-1)
        ptrD3[i] = 1.0;

      ptrD4[i] = rhsEval(x) * dx*dx;

      if (i==1)
        ptrD4[i] -= BC(xL);
      if (i==nInn-1)
        ptrD4[i] -= BC(xR);

      x += dx;		
    }
	});	
}


void solveTridiagonalSystem(AccessHandle<std::vector<double>> & subD,
														AccessHandle<std::vector<double>> & diag, 
														AccessHandle<std::vector<double>> & supD,
														AccessHandle<std::vector<double>> & rhs)
{
 	// solve tridiagonal system
 	create_work([=]
 	{
 		// int n=nInn;
 		// std::cout << n << std::endl;
 		double * pta = subD->data();
 		double * ptb = diag->data();
 		double * ptc = supD->data();
 		double * ptd = rhs->data();

 		solveThomas(pta, ptb, ptc, ptd, nInn);
 	});
}


void checkFinalL1Error(AccessHandle<std::vector<double>> & solution)
{
 	// check solution error 
 	create_work([=]
 	{
 		double * ptd = solution->data();

		double error = 0.0;
		double x = dx;
		for (int i = 0; i < (int) solution->size(); ++i)
		{
			error += std::abs( trueSolution(x) - ptd[i] );
			std::cout << " true = " << trueSolution(x) << " approx = " << ptd[i] << std::endl;
			x += dx;
		}
		std::cout << " L1 error = " << error << std::endl;
		assert( error < 1e-2 );
 	});
}


////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

  assert(n_spmd>=1);   // run with 1 rank, ignore the rest
  if (me>0){
    darma_finalize();
    return 0;
  }

  typedef std::vector<double> vecDbl;
	// handles with write privileges to vectors of doubles
	auto subD = initial_access<vecDbl>("a",me); // subdiagonal 
	auto diag = initial_access<vecDbl>("b",me); // diagonal 
	auto supD = initial_access<vecDbl>("c",me); // superdiagonal
	auto rhs  = initial_access<vecDbl>("d",me); // rhs and solution

	initialize(subD, diag, supD, rhs);
	solveTridiagonalSystem(subD, diag, supD, rhs);
	checkFinalL1Error(rhs);

  darma_finalize();
  return 0;
}


