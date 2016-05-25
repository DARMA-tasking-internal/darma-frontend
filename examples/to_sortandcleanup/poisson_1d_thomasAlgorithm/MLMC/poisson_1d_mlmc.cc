

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include "mpi.h"
#include <vector>
#include <random>

#include "../common_poisson1d.h"


#define MESS(msg) std::cout << msg << std::endl;
#define STOP(msg) assert(1==2);


/* 
	MLMC for 1d Poisson equation using 2d order FD in space.
		
	PDE: 
		k d^2 u(x)/dx^2 = g(x)
*/

using uDist = std::uniform_real_distribution<double>;


constexpr int lshift = 5; 	// # points at level l = 2^(l+lshift) + 1
constexpr double xL = 0.0;
constexpr double xR = 1.0;
constexpr double kmin = 0.8;
constexpr double kmax = 1.5;



double BC(double x)
{
	return exp(x) * sin(x);
}

double rhs(double x)
{
  return 2.0 * exp(x) * cos(x);
}


// solve PDE
double solveDiff1d(double sampleDiff, int nPt, double dx)
{
	int nInn = nPt-2;
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

		d[i] = rhs(x) * dx*dx / sampleDiff;

		if (i==1)
			d[i] -= BC(xL);
		if (i==nInn-1)
			d[i] -= BC(xR);

		x += dx;
	}

	solveThomas(a, b, c, d, nInn); 

	// take value at mid point 
	return d[(nInn-1)/2];
}




void mlmc_lev_l(int l, int N, double *sums, 
								std::mt19937 & generator, uDist & RNG)
{
  //      l       = level
  //      N       = number of samples
  //      sums[0] = sum(Y)
  //      sums[1] = sum(Y.^2)
  //      where Y are iid samples with expected value:
  //      E[P_0]           on level 0
  //      E[P_l - P_{l-1}] on level l>0

  double qoif = 0.0;
  double qoic = 0.0;
  double Yl = 0.0;
  for (int k=0; k<6; k++) 
    sums[k] = 0.0;

  for (int np = 0; np<N; np++) 
  {
    double sampleDiff = RNG(generator);
    // MESS(sampleDiff);

  	int nPtf = std::pow( 2.0, (double) l+lshift) + 1;
    // MESS( "nPtf=" << nPtf );
    double hf = (xR-xL)/static_cast<double>(nPtf);
    qoif = solveDiff1d(sampleDiff, nPtf, hf);

    if (l>=1)
    {
    	int nPtc = std::pow( 2.0, (double) l+lshift-1) + 1;
    	double hc = (xR-xL)/static_cast<double>(nPtc);
    	qoic = solveDiff1d(sampleDiff, nPtc, hc);
      // MESS("qoic=" << qoic << " qoif=" << qoif);
    }

    Yl = qoif - qoic;

    sums[0] += Yl;
    sums[1] += Yl*Yl;
  }
}



double mlmc(int Lmin, int Lmax, 
					 	int N0, double eps, 
					 	std::mt19937 & generator, uDist & RNG)
{
  int Nl[Lmax+1];
  int dNl[21], L;
  bool converged;
  double sums[6]; 
  double suml[3][21];
  double ml[21], Vl[21], Cl[21], x[21], y[21], alpha, beta, sum, theta;

  // Lmin >= 2
  // Lmax >= Lmin
  // N0 > 0 eps>0.0 
  L = Lmin;
  converged = false;
  for(int l=0; l<=Lmax; l++)
  {
    Nl[l] = 0;

    int nPtl = std::pow( 2.0, (double) l+lshift) + 1;
    double hl = (xR-xL)/static_cast<double>(nPtl);
    Cl[l] = hl; // cost at l ~ spacing

    for(int n=0; n<3; n++)
      suml[n][l] = 0.0;
  }

  // initial number of samples is N0 everywhere
  for(int l=0; l<=Lmin; l++)
    dNl[l] = N0;


  // // main loop
  while (!converged)
  {
    // update sample sums
    for (int l=0; l<=L; l++)
    {
      if (dNl[l]>0) 
      {
        mlmc_lev_l(l,dNl[l],sums, generator, RNG);
        suml[0][l] += (double) dNl[l];
        suml[1][l] += sums[0];
        suml[2][l] += sums[1];
      }
    }

    // compute absolute average and variance,
    // correct for possible under-sampling,
    // and set optimal number of new samples
    sum = 0.0f;
    for (int l=0; l<=L; l++) 
    {
      ml[l] = fabs(suml[1][l]/suml[0][l]);
      Vl[l] = fmaxf(suml[2][l]/suml[0][l] - ml[l]*ml[l], 0.0f);
      
      sum += sqrtf(Vl[l]/Cl[l]);
      MESS( "l=" << l << " ml=" << ml[l] << " Vl=" << Vl[l] );
    }
    MESS( "" );

    for (int l=0; l<=L; l++)
    {
      int nlNew = (2.0/(eps*eps)) * sqrtf(Vl[l]*Cl[l]) * sum; 
      dNl[l] = ceilf( fmaxf( 0.0f, nlNew-suml[0][l]) );
      MESS( "l=" << l << 
            " nlOld=" << suml[0][l] << 
            " nlNew=" << nlNew <<
            " dNl=" << dNl[l] );      
    }


    double rem = ml[L];
    if (rem > eps/sqrtf(2.0) ) 
    {
      MESS( " NOT conv " );

      if (L==Lmax)
        printf("*** failed to achieve weak convergence *** \n");
      else 
      {
        converged = false;
        L++;
        dNl[L] = N0;
      }
    }
    else
    {
      converged = true;
      MESS( " conv= " << converged << 
            " ml[L]=" << ml[L] << 
            " rem=" << rem <<
            " val=" << eps/sqrtf(2.0) );
    }
  }


  // finally, evaluate multilevel estimator
  double P = 0.0f;
  for (int l=0; l<=L; l++)
  {
    P    += suml[1][l]/suml[0][l];
    Nl[l] = suml[0][l];
    MESS( "l=" << l << " Nl=" << Nl[l]);
  }

  return P;
}



////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
  int N0 = 25;   // initial samples on each level
  int Lmin = 2;   // minimum refinement level
  int Lmax = 10;  // maximum refinement level
  double eps = 0.0001;

	std::random_device rd;
  std::mt19937 gen(rd());
  uDist RNG(kmin, kmax);

	double result = mlmc(Lmin, Lmax, N0, eps, gen, RNG);
  MESS( "result = " << result );
}
