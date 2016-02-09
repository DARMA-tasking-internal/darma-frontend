
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include <vector>
#include <iomanip>

#include <dharma.h>
using namespace dharma_runtime;


/* 
	Test problem for 1d heat equation with stochastic duffusivity. 
	Very simple monte carlo analysis. 

	1d heat equation using forward Euler in time, 2d order FD in space.
	PDE: 
		dT/dt = alpha * dT^2/dx^2

	over (0,1), with T(0)=100, T(1)=10

	Assume alpha = U(0.0070, 0.0080)   uniform distribution.

	Monte carlo: sample alpha, solve PDE, collect results, do statistics. 

*/


constexpr int numMCSamples = 100; 	// num of Monte Carlo samples

constexpr int n_iter 	= 500; 	// num of iterations in time
constexpr double deltaT = 0.0025;	// time step 
constexpr double alphaMin  = 0.007;	// min value of diffusivity
constexpr double alphaMax  = 0.008;	// max value of diffusivity

constexpr int nx = 100; 			// number of grid points  
constexpr double x_min = 0.0; 		// domain start x 
constexpr double x_max = 1.0; 		// domain end x
constexpr double deltaX = (x_max-x_min)/( (double) (nx-1) ); 	// grid cell spacing
constexpr double cfl1 = alphaMin * deltaT / (deltaX * deltaX ); 	// cfl condition for stability
static_assert( cfl1 < 0.5, "cfl not small enough");	
constexpr double cfl2 = alphaMax * deltaT / (deltaX * deltaX ); 	// cfl condition for stability
static_assert( cfl2 < 0.5, "cfl not small enough");	

// need to decide what is chunk of data that pertain to a task 
constexpr int numPtsPerTask = 25;
static_assert( nx % numPtsPerTask ==0 , "nx % numPtsPerTask != 0");
constexpr int numPtsPerTaskWithGhosts = numPtsPerTask + 2;




int main(int argc, char** argv)
{
	const size_t myPID  = dharma_spmd_rank();
	const size_t n_spmd = dharma_spmd_size();

	// Figure out my neighbors.  If I'm 0 or n_spmd-1, I am my own
	// neighbor on the left and right
	const bool is_leftmost = myPID == 0;
	const size_t left_neighbor = is_leftmost ? myPID : myPID - 1;
	const bool is_rightmost = myPID == n_spmd - 1;
	const size_t right_neighbor = myPID == is_rightmost ? myPID : myPID + 1;

	// -----------------------------------------------------
	
	// rank 0 creates an array of random diffusivities 
	if ( myPID == 0 )
	{
 	 	auto alphaset = initial_access<std::vector<double>>("alpha_values", myPID);
	 	create_work([=]
    {
  	 	alpha_vec->resize(numMCSamples);
  		double* data_ptr = alpha_vec->data();

  		// fill in stochastic parameter drawn from U(0.0070, 0.0080)
  		// todo

    });

	 	// how many read_access handles will be created
 		alphaset.publish(n_readers=n_spmd);
	}

	// -----------------------------------------------------

	// create initial condition
  auto data = initial_access<std::vector<double>>("initial_data", me);
  auto gv_to_left = initial_access<double>("ghost_value_for_left_neigh", me, 0);
  auto gv_to_right = initial_access<double>("ghost_value_for_right_neigh", me, 0);
 	create_work([=]
 	{
 		data->resize(num_points_per_rank);

 		double * data_ptr = data->data();
		for (int i = 0; i < num_points_per_rank; ++i)
			data_ptr[i] = 50.0;

    if(is_leftmost)
    	data_ptr[0] = Tl;
    if(is_rightmost) 
    	data_ptr[num_points_per_rank-1] = Tr;

    // all tasks need to set the values of the ghosts 
    gv_to_left.set_value(data_ptr[0]);
    gv_to_right.set_value(data_ptr[num_points_per_rank-1]);

	});

 	// this means that below create work only capture read_access to data 
 	// data can only be read below
 	data = read_access_to(data);

	// -----------------------------------------------------

	// each rank creates an access to the random diffusivities 
	auto alphaVecRead = read_access<std::vector<double>>("alpha_values", 0);

	// Monte Carlo loop
	for (int iSampl = 0; iSampl < numMCSamples; ++iSampl)
	{
  	auto dataNew = initial_access<std::vector<double>>("data", me, iSampl); 			
  	auto gv_to_left_iSampl = initial_access<double>("ghost_value_for_left_neigh", me, 0, iSampl);
  	auto gv_to_right_iSampl = initial_access<double>("ghost_value_for_right_neigh", me, 0, iSampl);
	 	create_work([=]
	 	{
	 		// copy data into dataNew
	 		dataNew->reserve(data->size());
	 		memcpy(dataNew->data(), data->data(), data->size()*sizeof(double));

	 		gv_to_left_iSampl.set_value( gv_to_left.get_value() );
	 		gv_to_right_iSampl.set_value( gv_to_right.get_value() );

		});

	 	gv_to_left_iSampl.publish(n_readers=1);
 	 	gv_to_right_iSampl.publish(n_readers=1);
 	 	gv_to_left_iSampl = nullptr;
		gv_to_right_iSampl = nullptr;


	 	create_work([=]
	 	{
	 		// this is the diffusivity for this particular MC sample
			double alphadtOvdxSq = (alphaVecRead->data()[iSampl] * deltaT) / (deltaX * deltaX );   // alpha * DT/ DX^2

			// time loop 
			for (int timeLoop = 0; timeLoop < n_iter; ++timeLoop)
			{
		  	auto gv_from_left_neigh  = read_access<double>("ghost_value_for_right_neigh", left_neighbor, timeLoop, iSampl);
		  	auto gv_from_right_neigh = read_access<double>("ghost_value_for_left_neigh", right_neighbor, timeLoop, iSampl);

		  	auto gv_to_left_iSampl_tl  = initial_access<double>("ghost_value_for_left_neigh", me, timeLoop+1, iSampl);
		  	auto gv_to_right_iSampl_tl = initial_access<double>("ghost_value_for_right_neigh", me, timeLoop+1, iSampl);

		 		create_work([=]
		 		{
		  		std::vector<double> local_T_wghosts(num_points_per_rank_wghosts, 0.0);
		  		local_T_wghosts[0] 	= gv_from_left_neigh.get_value();
		  		local_T_wghosts[num_points_per_rank_wghosts-1] = gv_from_right_neigh.get_value();

		  		for (int i = 1; i <= num_points_per_rank; ++i)
		  		{
						local_T_wghosts[i] = dataNew->data()[i-1];
		  		}

					// update field only for inner points based on FD stencil 
			    for (int i = 1; i <= num_points_per_rank; i++ )
			    {
			    	const double FDLapStencil = ( local_T_wghosts[i+1] - 2.0*local_T_wghosts[i] + local_T_wghosts[i-1] );
						dataNew->data()[i] = local_T_wghosts[i] + alphadtOvdxSq * FDLapStencil;
			    }

			    gv_to_left_iSampl_tl.set_value( dataNew->data()[0] );
			    gv_to_right_iSampl_tl.set_value( dataNew->data()[num_points_per_rank-1] );
		 		});

		 	 	gv_to_left_iSampl_tl.publish(n_readers=1);
		 	 	gv_to_right_iSampl_tl.publish(n_readers=1);
			}	
		});

	}

  dharma_finalize();

}// end main

