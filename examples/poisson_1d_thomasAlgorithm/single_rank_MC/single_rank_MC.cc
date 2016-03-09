

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include <vector>
#include <iomanip>
#include <random>

#include "../common_poisson1d.h"
#include <darma.h>


/* 
	Monte Carlo for 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		k d^2 u(x)/dx^2 = g(x)
	over (0,1). 

	k ~ U(1.0, 1.1)
*/

using uDist = std::uniform_real_distribution<double>;
constexpr double kmin = 1.0;
constexpr double kmax = 1.1;
constexpr int numMCRuns = 50;



////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
	using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

	std::random_device rd;
  std::mt19937 gen(rd());
  uDist RNG(kmin, kmax);

  // grid related info 
	constexpr int nx = 51; 
	constexpr int nInn = nx-2;
	constexpr double xL = 0.0;
	constexpr double xR = 1.0;
	constexpr double dx = 1.0/static_cast<double>(nx-1);

	// vectors that make up the FD matrix
	auto subD = initial_access<std::vector<double>>("a",me); // subdiagonal 
	auto diag = initial_access<std::vector<double>>("b",me); // diagonal 
	auto supD = initial_access<std::vector<double>>("c",me); // superdiagonal

	// vector to collect data from MC results
	auto mcData = initial_access<std::vector<double>>("MC_results",me); // collects results from MC runs

	// initialize the FD matrix
 	create_work([=]
 	{
 		subD.emplace_value();	// this will go away eventually
 		diag.emplace_value();
 		supD.emplace_value();
 		subD->resize(nInn);
 		diag->resize(nInn,0.0);
 		supD->resize(nInn,0.0);
 		double * pt1 = subD->data();
 		double * pt2 = diag->data();
 		double * pt3 = supD->data();

		double x = dx;
		for (int i = 0; i < nInn; ++i)
		{
			pt2[i] = -2;
			if (i>0)
				pt1[i] = 1.0;
			if (i<nInn-1)
				pt3[i] = 1.0;
			x += dx;
		}

		mcData.emplace_value();
		mcData->resize(numMCRuns,0.0);
	});
  subD.publish(n_readers=numMCRuns);
  diag.publish(n_readers=numMCRuns);
  supD.publish(n_readers=numMCRuns);


 	for (int iMC = 0; iMC < numMCRuns; ++iMC)
 	{
    double sampleDiff = RNG(gen);
    auto c = initial_access<std::vector<double>>("cmc", me, iMC);
		auto rhs = initial_access<std::vector<double>>("dmc",me,iMC); // rhs and solution
    auto cold = read_access<std::vector<double>>("c", me);

	 	create_work([=]
	 	{
	 		double * ptCold = cold->data();
	 		c.emplace_value(cold.get_value()); 

	 		rhs.emplace_value();
	 		rhs->resize(nInn,0.0);
	 		double * pt4 = rhs->data();
			double x = dx;
			for (int i = 0; i < nInn; ++i)
			{
				pt4[i] = rhsEval(x) * dx*dx / sampleDiff;
				if (i==1)
					pt4[i] -= trueSolution(xL);
				if (i==nInn-1)
					pt4[i] -= trueSolution(xR);
				x += dx;
			}
	 	});

  	c.publish(n_readers=1);
  	rhs.publish(n_readers=1);
 	}


 	for (int iMC = 0; iMC < numMCRuns; ++iMC)
 	{
    auto a = read_access<std::vector<double>>("a", me);
    auto b = read_access<std::vector<double>>("b", me);
    auto c = read_access<std::vector<double>>("cmc", me, iMC);
    auto d = read_access<std::vector<double>>("dmc", me, iMC);

	 	create_work([=]
	 	{
	 		double * pta = a->data(); //not modified by thomas 
	 		double * ptb = b->data();	//not modified by thomas 
	 		double * ptc = c->data();	//need a copy because it is modified by thomas 
	 		double * ptd = d->data(); // this contasins rhs and result 

	 		solveThomas(pta, ptb, ptc, ptd, nInn);

	 		double * mcDataPt = mcData->data();

	    // auto mcRes = read_write<std::vector<double>>("MC_results", me);
	    double * mcResPtr = mcData->data();
	 		mcResPtr[iMC] = ptd[(nInn-1)/2];
	 		std::cout << ptd[(nInn-1)/2] << std::endl;
	 	});
 	}


  darma_finalize();
  return 0;
}

