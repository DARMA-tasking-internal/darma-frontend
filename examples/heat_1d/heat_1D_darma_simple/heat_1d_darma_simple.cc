

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include <vector>
#include <iomanip>

#include "../common_heat1d.h"

#include <threads_backend.h>
#include <darma.h>


/* 
	Solve 1d heat equation using forward Euler in time, 2d order FD in space.
		
	PDE: 
		dT/dt = alpha * dT^2/dx^2

	over (0,1), with T(0)=100, T(1)=10
		
*/

////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
 	darma_init(argc, argv);

	using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

	const size_t me  = darma_spmd_rank();
	const size_t n_spmd = darma_spmd_size();

	// -----------------------------------------------------
	// Figure out my neighbors. 0 or n_spmd-1, I am my own neighbor
	const bool is_leftmost = me == 0;
	const size_t left_neighbor = is_leftmost ? me : me - 1;
	const bool is_rightmost = me == n_spmd - 1;
	const size_t right_neighbor = me == is_rightmost ? me : me + 1;

	// -----------------------------------------------------
	assert( nx % n_spmd == 0 );
	const int num_points_per_rank = nx / n_spmd; 
	const int num_points_per_rank_wghosts = num_points_per_rank + 2; 

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

	// need to publish when something potentially goes off node	
 	gv_to_left.publish(n_readers=1); 		// delete after one read access handle is created
 	gv_to_right.publish(n_readers=1); 	// delete after one read access handle is created


	// -----------------------------------------------------

	// time loop 
	for (int timeLoop = 0; timeLoop < n_iter; ++timeLoop)
	{
		// handle with read privileges
  	auto gv_from_left_neigh  = read_access<double>("ghost_value_for_right_neigh", left_neighbor, timeLoop);
		// handle with read privileges
  	auto gv_from_right_neigh = read_access<double>("ghost_value_for_left_neigh", right_neighbor, timeLoop);

  	gv_to_left = initial_access<double>("ghost_value_for_left_neigh", me, timeLoop);
  	gv_to_right = initial_access<double>("ghost_value_for_right_neigh", me, timeLoop);

 		create_work([=]
 		{
  		std::vector<double> local_T_wghosts(num_points_per_rank_wghosts, 0.0);
  		local_T_wghosts[0] 	= gv_from_left_neigh.get_value();
  		local_T_wghosts[num_points_per_rank_wghosts-1] = gv_from_right_neigh.get_value();

  		for (int i = 1; i <= num_points_per_rank; ++i)
  		{
				local_T_wghosts[i] = data->data()[i-1];
  		}

			// update field only for inner points based on FD stencil 
	    for (int i = 1; i <= num_points_per_rank; i++ )
	    {
	    	const double FDLapStencil = ( local_T_wghosts[i+1] - 2.0*local_T_wghosts[i] + local_T_wghosts[i-1] );
				data->data()[i] = local_T_wghosts[i] + alphadtdxSq * FDLapStencil;
	    }

	    gv_to_left.set_value( data[0] );
	    gv_to_right.set_value( data[num_points_per_rank-1] );

 		});

 	 	gv_to_left.publish(n_readers=1);
 	 	gv_to_right.publish(n_readers=1);

  }	

  darma_finalize();

}//end main
