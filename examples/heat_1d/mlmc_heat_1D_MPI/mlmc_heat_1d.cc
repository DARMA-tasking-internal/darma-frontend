

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include "mpi.h"
#include <random>
#include <vector>

constexpr double x_min = 0.0;     // domain start x 
constexpr double x_max = 1.0;     // domain end x
constexpr double Tinn = 50.0;     // init temperature inner points
constexpr double Tl = 100.0;     	// left BC for temperature
constexpr double Tr = 10.0;      	// right BC for temperature
constexpr double runTime = 1.25;   // final time 
constexpr double cfl = 0.01;   			// cfl number 
constexpr double thermDiffMin  = 0.07;	// min value of diffusivity
constexpr double thermDiffMax  = 0.08;	// max value of diffusivity

constexpr int mlmcN0 = 50;	// initial N samples for MLMC
constexpr double epsilon = 1e-3;	// epsilon, target tolerance for statistics
constexpr double invepssq = std::pow(epsilon, -2.0);	// 1/epsilon^2

using gen = std::default_random_engine;
using uDist = std::uniform_real_distribution<double>;

////////////////////////////////////////////////////////////////////////////////

double computeMean( const std::vector<double> array)
{
	double mean = 0.0; 
  for ( const auto & it : array)
      mean += it;
  return mean/(double) array.size();
}

double computeVariance( const std::vector<double> array)
{
	double mean = computeMean(array);
	double sum = 0.0; 
  for ( const auto & it : array)
      sum += pow( it - mean, 2.0 );
  return sum/(double) (array.size()-1);
}

void initializeTempField(int nx, std::vector<double> & tempField)
{
	assert( tempField.size() == nx );
	std::fill( tempField.begin(), tempField.end(), Tinn );
	tempField[0] = Tl;
	tempField[nx-1] = Tr;
}

void solvePDE(int nx, double thermDiff, std::vector<double> & tempField)
{
	assert( tempField.size() == nx );
	// setup
	const double deltaX = (x_max-x_min)/( (double) (nx-1) );  // grid cell spacing
	double deltaT = cfl * deltaX * deltaX / thermDiff;  			// time step
	const int nIter = std::ceil( runTime / deltaT );
	deltaT = runTime / nIter;																	// time step
	const double alphadtOvdxSq = (thermDiff*deltaT)/(deltaX * deltaX );   // alpha * DT/ DX^2

	initializeTempField(nx, tempField);

	std::vector<double> oldTemp(tempField); 
	for (int timeLoop = 1; timeLoop <= nIter; ++timeLoop)
	{
		for (int i=1; i<=nx-2; i++)
		{
			double laplacian = ( oldTemp[i+1] - 2.0*oldTemp[i] + oldTemp[i-1] );
			tempField[i] = oldTemp[i] + alphadtOvdxSq * laplacian;
		}
		oldTemp = tempField;
	}
}

double extractQoIMidPlane(int nx, const std::vector<double> & tempField)
{
	// grids are of the form: 2^l + 1 
	// so mid location is x_mid is at i= (nx-1)/2 
	const int midInd = (nx-1)/2;
	return tempField[midInd];
}

void drawSamplesFromUniform(gen & generator, 
														uDist & RNG, 
														int N, 
														std::vector<double> & realizations)
{	
	realizations.resize(N);
	for (int i = 0; i < N; ++i)
	{
		realizations[i] = RNG(generator);
	}
}

struct mlmcLevel
{
  int mcmcL_;
  int Nsamples_;
  int optiN_;
  int NgridPt_;
  double h_;
  std::vector<double> alphaSamples_;
  std::vector<double> qoiSamples_;
  double variance_;
};

void collectSamplesAtLevel(mlmcLevel & level)
{
	std::vector<double> solution(level.NgridPt_,0.0);
	for (int i = 0; i < level.Nsamples_; ++i)
	{
		solution.resize(level.NgridPt_, 0.0);
		solvePDE(level.NgridPt_, level.alphaSamples_[i], solution);
		level.qoiSamples_.push_back( extractQoIMidPlane(level.NgridPt_, solution));
	}
}

double computeVL(const std::vector<mlmcLevel> & allLevels)
{
	double result = 0.0;
	for (auto & iLev : allLevels)
	{
		iLev.variance_ = computeVariance(iLev.qoiSamples_);
		result += iLev.variance_;
	}
	return result;
}


// void calculateOptimalNAllLevels(std::vector<mlmcLevel> & allLevels)
// {	
// 	int numLevels = allLevels.size();

// 	std::vector<double> Vl(numLevels);
// 	for (int i = 0; i < numLevels; ++i)
// 	{
// 		Vl[i] = computeVariance(allLevels[i].qoiSamples_);
// 	}

// 	int k = 0;
// 	for (auto & iLev : allLevels)
// 	{
// 		double f1 = 2.0*invepssq * sqrt( Vl[k]*iLev.h_ );
// 		calculateOptimalN(iLev);
// 		k++;
// 	}
// }



////////////////////////////////////////////////////////////////////////////////
// main() function
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	gen generator;
  uDist RNG(thermDiffMin,thermDiffMax);

	// std::vector<double> temp(65);
	// solvePDE(65, 0.075, temp);
	// for (auto & it : temp)
	// 	std::cout << it << std::endl;


  mlmcLevel lev0;
  lev0.mcmcL_ = 0;
  lev0.Nsamples_ = mlmcN0;
  lev0.NgridPt_ = 65; // 2^6+1
  lev0.h_ = (x_max-x_min)/( (double) (lev0.NgridPt_-1) );

	drawSamplesFromUniform(generator, RNG, mlmcN0, lev0.alphaSamples_);
	collectSamplesAtLevel(lev0);
  std::vector<mlmcLevel> allLevels(lev0);

	double VL = computeVL(allLevels);

	// calculateOptimalNAllLevels(allLevels);

	// for (auto & it : lev0.qoiSamples_)
	// 	std::cout << it << std::endl;
	// std::cout << std::endl;
 //  std::cout << computeMean(lev0.qoiSamples_) << " " << computeVariance(lev0.qoiSamples_) << std::endl;


}

