#include <cmath>
#include <darma.h>
#include "../common_heat1d.h"

/* 
	Full grid: 
	    	o  o  o  o  o  o  o  o  o  o  o  o  o  o  o  o
		
	Distribute uniformly accross all ranks: 

     +  o  o  o  o  *
			     			 *  o  o  o  o  *
							 							 *  o  o  o  o  *
										 										 *  o  o  o  o  +

		   		r0		   			r1 					r2         	r3

	Locally, each rank owns elements: 
			
		*  o  o  o  o  *

	where inner points are: o 
		    ghosts points are: * 

	The points denoted with + are not needed because outside of domain, 
	but exist anyway so that each local vector has same size.
	
	Below we use following shortcut for indices of key points:
	
		*    o    o   o    o   *
		lli  li 		  		ri  rri
*/

int darma_main(int argc, char** argv)
{
	using namespace darma_runtime;
	using namespace darma_runtime::keyword_arguments_for_publication;
 	darma_init(argc, argv);
	const size_t me  = darma_spmd_rank();
	const size_t n_spmd = darma_spmd_size();
	// supposed to be run with 4 ranks
	if (n_spmd!=4){
		std::cerr << "# of ranks != 4, not supported!" << std::endl;
    std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
    exit( EXIT_FAILURE );
	}

	// Figure out my neighbors. 0 or n_spmd-1, I am my own neighbor
	const bool is_leftmost = me == 0;
	const size_t left_neighbor = is_leftmost ? me : me - 1;
	const bool is_rightmost = me == n_spmd - 1;
	const size_t right_neighbor = is_rightmost ? me : me + 1;
	assert( nx % n_spmd == 0 ); 	// same number of points locally
	const int num_points_per_rank = nx / n_spmd; 
	const int num_points_per_rank_wghosts = num_points_per_rank + 2; 
	const int num_cells_per_rank = num_points_per_rank-1;

	// useful to identify local grid points
	const int lli = 0;
	const int li  = 1;
	const int ri  = num_points_per_rank;
	const int rri = num_points_per_rank+1;

	// left boundary of my local part of the grid 
	const double xL = is_leftmost ? 0.0 : num_points_per_rank * deltaX * me; 



	/**********************************************************
					initialize temp field and ghost values 
	**********************************************************/		

	// handle to my data
  auto data = initial_access<std::vector<double>>("data", me);
	// handle to ghost value for my left neighbor
  auto gv_to_left = initial_access<double>("ghost_for_left_neigh", me, 0);
	// handle to ghost value for my right neighbor
  auto gv_to_right = initial_access<double>("ghost_for_right_neigh", me, 0);

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
	// publish only the ghost points, since data remains local	
	gv_to_left.publish(n_readers=1);		
	gv_to_right.publish(n_readers=1);


	/**********************************************************
													Time loop
	**********************************************************/		
	for (int iLoop = 0; iLoop < n_iter; ++iLoop)
	{
 		auto gv_from_left_neigh 
 			= is_leftmost ? read_access<double>("ghost_for_left_neigh",me,iLoop) : 
 					 read_access<double>("ghost_for_right_neigh",left_neighbor,iLoop);

 		auto gv_from_right_neigh 
 			= is_rightmost ? read_access<double>("ghost_for_right_neigh",me,iLoop) : 
 						read_access<double>("ghost_for_left_neigh",right_neighbor,iLoop);

		gv_to_left = initial_access<double>("ghost_for_left_neigh",me,iLoop+1);
  	gv_to_right = initial_access<double>("ghost_for_right_neigh",me,iLoop+1);

 		create_work([=]
 		{
 			auto & dataRef = data.get_reference();
  		std::vector<double> my_T_wghosts(dataRef);
			my_T_wghosts[lli] 	= gv_from_left_neigh.get_value();
			my_T_wghosts[rri] = gv_from_right_neigh.get_value();

			// update field only for inner points based on FD stencil 
	    for (int i = li; i <= ri; i++ )
	    {
	    	double FD = my_T_wghosts[i+1]-2.0*my_T_wghosts[i]+my_T_wghosts[i-1];
				dataRef[i] = my_T_wghosts[i] + alphadtOvdxSq * FD;
	    }

	    // fix the domain boundary conditions 
	    if(is_leftmost)
	    	dataRef[li] = Tl;
	    if(is_rightmost) 
	    	dataRef[ri] = Tr;

	    gv_to_left.set_value( dataRef[li] );
	    gv_to_right.set_value( dataRef[ri] );
 		});

    if (iLoop < n_iter-1){
        gv_to_left.publish(n_readers=1);
        gv_to_right.publish(n_readers=1);
    }

  }	//time loop

	/**********************************************************
									Check convergence & print  
	**********************************************************/		

  // calculate error locally 
  auto myErr = initial_access<double>("myl1error", me);
  // need a separate variable for the collective result
  auto myGlobalErr = initial_access<double>("globall1error", me);
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
		myErr.set_value(error);
		myGlobalErr.set_value(error);
  });
  // will be read by all ranks except myself
	myErr.publish(n_readers=n_spmd-1);

	// each rank performs global sum: mimicing collective
	for (int iPd = 0; iPd < n_spmd; ++iPd)
	{
		if (iPd != me)
		{
 	 		auto iPdErr = read_access<double>("myl1error",iPd);
			create_work([=]
			{
				myGlobalErr.get_reference() += iPdErr.get_value();
 	 		});
		}
	}

  create_work([=]
  {
    std::stringstream ss;
    ss << " global L1 error = " << myGlobalErr.get_value() << std::endl;
    std::cout << ss.str();
		if (myGlobalErr.get_value() > 1e-2)
		{
			std::cerr << "PDE solve did not converge: L1 error > 1e-2" << std::endl;
	    std::cerr << " " __FILE__ << ":" << __LINE__ << '\n';        
	    exit( EXIT_FAILURE );
		}
  });

  darma_finalize();
  return 0;

}//end main