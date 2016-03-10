

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
*/

////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
	using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;
  // using namespace darma_runtime::keyword_arguments_for_create_work_decorators;

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

	// useful constants (see above for legend )
	const int lli = 0;
	const int li  = 1;
	const int ri  = num_points_per_rank;
	const int rri = num_points_per_rank+1;

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
 		data->resize(num_points_per_rank_wghosts, 0.0);
 		auto & vecRef = data.get_reference();
 		for (auto & it : vecRef)
 			it = 50.0;

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
 		// int timeLoop = 0;

 		// fake it for now, just so that something is initialized 
 		AccessHandle<double> gv_from_left_neigh = initial_access<double>("fake", me, timeLoop);
 		AccessHandle<double> gv_from_right_neigh = initial_access<double>("fake2", me, timeLoop);
		if (!is_leftmost)
  		gv_from_left_neigh  = read_access<double>("ghost_value_for_right_neigh", left_neighbor, timeLoop);
  	if (!is_rightmost)
  		gv_from_right_neigh = read_access<double>("ghost_value_for_left_neigh", right_neighbor, timeLoop);

		if (!is_leftmost)
  		gv_to_left = initial_access<double>("ghost_value_for_left_neigh", me, timeLoop+1);
  	if (!is_rightmost)
	  	gv_to_right = initial_access<double>("ghost_value_for_right_neigh", me, timeLoop+1);


		if (me==1)
			sleep(1);
 		create_work([=]
 		{
 			// auto & vecRef = data.get_reference();

 			double * data_ptr = data->data();

  		std::vector<double> local_T_wghosts(num_points_per_rank_wghosts, 0.0);
			if (!is_leftmost)
  			local_T_wghosts[lli] 	= gv_from_left_neigh.get_value();
  		if (!is_rightmost)
  			local_T_wghosts[rri] = gv_from_right_neigh.get_value();

  		for (int i = 1; i <= num_points_per_rank; ++i)
  		{
				local_T_wghosts[i] = data_ptr[i-1];
  		}

			// update field only for inner points based on FD stencil 
	    for (int i = 1; i <= num_points_per_rank; i++ )
	    {
	    	const double FDLapStencil = ( local_T_wghosts[i+1] - 2.0*local_T_wghosts[i] + local_T_wghosts[i-1] );
				data_ptr[i] = local_T_wghosts[i] + alphadtOvdxSq * FDLapStencil;
	    }

	    // fix the domain boundary conditions 
	    if(is_leftmost)
	    	data_ptr[li] = Tl;
	    if(is_rightmost) 
	    	data_ptr[ri] = Tr;

	    gv_to_left.set_value( data_ptr[li] );
	    gv_to_right.set_value( data_ptr[ri] );

 		});

		if (!is_leftmost)
 	 		gv_to_left.publish(n_readers=1);
 		if (!is_rightmost) 	 	
 	 		gv_to_right.publish(n_readers=1);

  }	

	if (me==1)
		sleep(1);

  create_work([=]
  {
		double * data_ptr = data->data();
	  for ( int i = 1; i <= num_points_per_rank; i++ )
	  {
			std::cout << me << " " << data_ptr[i] << std::endl;
	  }
  });

  darma_finalize();
  return 0;

}//end main



/* 
	what is this? 

    create_work(reads(left_ghost, unless=is_leftmost),
      reads(right_ghost, unless=is_rightmost),


*/
