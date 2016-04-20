

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include <vector>
#include <iomanip>
#include <unistd.h>

#include "../common_heat1d.h"

#include <darma.h>


/* 
	Solve 1d heat equation using forward Euler in time, 2d order FD in space.
		
	PDE: 
		dT/dt = alpha * dT^2/dx^2

	over (0,1), with T(0)=100, T(1)=10		

	Full grid: 
	    	o  o  o  o  o  o  o  o  o  o  o  o  o  o  o  o
		
	Distribute uniformly accross all ranks: 

     s  o  o  o  o  *
			     			 *  o  o  o  o  *
							 							 *  o  o  o  o  *
										 										 *  o  o  o  o  s

		   		r0		   			r1 					r2         	r3

	s:  are not actually needed because outside of domain, 
		but exist anyway so that each local vector has same size.

	Locally, each rank owns elements: 
			
		*  o  o  o  o  *

	where inner points are: o 
		   ghosts points are: * 
	
	Below we use following shortcut for indices of key points:
	
		*    o    o   o    o   *
		lli  li 		  		ri  rri

*/

////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
	using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

 	darma_init(argc, argv);

	const size_t me  = darma_spmd_rank();
	const size_t n_spmd = darma_spmd_size();

	// -----------------------------------------------------
	// Figure out my neighbors. 0 or n_spmd-1, I am my own neighbor
	const bool is_leftmost = me == 0;
	const size_t left_neighbor = is_leftmost ? me : me - 1;
	const bool is_rightmost = me == n_spmd - 1;
	const size_t right_neighbor = is_rightmost ? me : me + 1;

	// -----------------------------------------------------

	assert( nx % n_spmd == 0 );
	const int num_points_per_rank = nx / n_spmd; 
	const int num_points_per_rank_wghosts = num_points_per_rank + 2; 
	const int num_cells_per_rank = num_points_per_rank-1;

	// useful constants (see above for legend )
	const int lli = 0;
	const int li  = 1;
	const int ri  = num_points_per_rank;
	const int rri = num_points_per_rank+1;
	const double xL = is_leftmost ? 0.0 : num_points_per_rank * deltaX * me; 

	// -----------------------------------------------------

	// handle with write privileges to a vector of doubles
  auto data = initial_access<std::vector<double>>("data", me);
	// handle with write privileges to a double
  auto gv_to_left = initial_access<double>("ghost_value_for_left_neigh", me, 0);
	// handle with write privileges to a double
  auto gv_to_right = initial_access<double>("ghost_value_for_right_neigh", me, 0);

	/* 	
		create the initial temp field and the ghost values 
	*/		
 	create_work([=]
 	{
 		data.emplace_value();
 		data->resize(num_points_per_rank_wghosts, 50.0);
 		auto & vecRef = data.get_reference();

    if(is_leftmost)
    	vecRef[li] = Tl;
    if(is_rightmost) 
    	vecRef[ri] = Tr;

    // all tasks need to set the values of the ghosts 
    gv_to_left.set_value(vecRef[li]);
    gv_to_right.set_value(vecRef[ri]);

	});

	// need to publish when something potentially goes off node	
	if (!is_leftmost)
 		gv_to_left.publish(n_readers=1); 		// delete after one read access handle is created
 	if (!is_rightmost)
 		gv_to_right.publish(n_readers=1); 	// delete after one read access handle is created

	// -----------------------------------------------------

	// time loop 
	for (int timeLoop = 0; timeLoop < n_iter; ++timeLoop)
	{
 		// this handle has to be initialized in all cases, so just set default to fake value
 		AccessHandle<double> gv_from_left_neigh  = initial_access<double>("fake", me, timeLoop);
 		AccessHandle<double> gv_from_right_neigh = initial_access<double>("fake2", me, timeLoop);
		if (!is_leftmost)
  		gv_from_left_neigh  = read_access<double>("ghost_value_for_right_neigh", left_neighbor, timeLoop);
  	if (!is_rightmost)
  		gv_from_right_neigh = read_access<double>("ghost_value_for_left_neigh", right_neighbor, timeLoop);

		if (!is_leftmost)
  		gv_to_left = initial_access<double>("ghost_value_for_left_neigh", me, timeLoop+1);
  	if (!is_rightmost)
	  	gv_to_right = initial_access<double>("ghost_value_for_right_neigh", me, timeLoop+1);

 		create_work([=]
 		{
 			auto & dataRef = data.get_reference();

  		std::vector<double> local_T_wghosts(dataRef);
			if (!is_leftmost)
  			local_T_wghosts[lli] 	= gv_from_left_neigh.get_value();
  		if (!is_rightmost)
  			local_T_wghosts[rri] = gv_from_right_neigh.get_value();

			// update field only for inner points based on FD stencil 
	    for (int i = li; i <= ri; i++ )
	    {
	    	double FDLapStencil = ( local_T_wghosts[i+1] - 2.0*local_T_wghosts[i] + local_T_wghosts[i-1] );
				dataRef[i] = local_T_wghosts[i] + alphadtOvdxSq * FDLapStencil;
	    }

	    // fix the domain boundary conditions 
	    if(is_leftmost)
	    	dataRef[li] = Tl;
	    if(is_rightmost) 
	    	dataRef[ri] = Tr;

	    gv_to_left.set_value( dataRef[li] );
	    gv_to_right.set_value( dataRef[ri] );

 		});

    if (timeLoop < n_iter-1){
      if (!is_leftmost)
        gv_to_left.publish(n_readers=1);
      if (!is_rightmost)
        gv_to_right.publish(n_readers=1);
    }
  }	

  // calculate error locally 
  auto myErrHandle = initial_access<double>("myl1error", me);
  // need a separate variable for the collective result to avoid deadlock
  auto myGlobalErrHandle = initial_access<double>("globall1error", me);
  create_work([=]
  {
		const auto & vecRef = data.get_reference();

		// only compute error for internal points
		double error = 0.0;
		for (int i = li; i <= ri; ++i)
		{
			double xx = xL+(i-1)*deltaX;
			error += std::abs( steadySolution(xx) - vecRef[i] );
		}
		myErrHandle.set_value(error);
		myGlobalErrHandle.set_value(error);
  });
  // will be read by all ranks except me
	myErrHandle.publish(n_readers=n_spmd-1,version=0);

	// fake collective procedure (each rank performs global sum)
	for (int iPd = 0; iPd < n_spmd; ++iPd)
	{
		if (iPd != me)
		{
 	 		auto iPdErr = read_access<double>("myl1error",iPd,version=0);
			create_work([=]
			{
				myGlobalErrHandle.get_reference() += iPdErr.get_value();
 	 		});
		}
	}

	// print to terminal
  create_work([=]
  {
		sleep(me+0.5);
		std::cout << " global L1 error = " << myGlobalErrHandle.get_value() << std::endl;		
		assert( myGlobalErrHandle.get_value() < 1e-2 );
	});

  darma_finalize();
  return 0;

}//end main

