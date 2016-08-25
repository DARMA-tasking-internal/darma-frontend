#include <cmath>
#include <darma.h>
#include "../../../0.2/heat_1d/common_heat1d.h"
using namespace darma_runtime;
#include "functors.h"

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

	// create object for storing aux vars
	aux_vars aux;

	// Figure out my neighbors. 0 or n_spmd-1, I am my own neighbor
	aux.is_leftmost = me == 0;
	aux.left_neighbor = aux.is_leftmost ? me : me - 1;
	aux.is_rightmost = me == n_spmd - 1;
	aux.right_neighbor = aux.is_rightmost ? me : me + 1;
	assert( nx % n_spmd == 0 ); 	// same number of points locally
	aux.num_points_per_rank = nx / n_spmd; 
	aux.num_points_per_rank_wghosts = aux.num_points_per_rank + 2; 
	aux.num_cells_per_rank = aux.num_points_per_rank-1;
	// useful to identify local grid points
	aux.lli = 0;
	aux.li  = 1;
	aux.ri  = aux.num_points_per_rank;
	aux.rri = aux.num_points_per_rank+1;
	// left boundary of my local part of the grid 
	aux.xL = aux.is_leftmost ? 0.0 : aux.num_points_per_rank * deltaX * me; 

	/**********************************************************
					initialize temp field and ghost values 
	**********************************************************/		

	// handle to my data
  auto data = initial_access<std::vector<double>>("data", me);
	// handle to ghost value for my left neighbor
  auto gv_to_left = initial_access<double>("ghost_for_left_neigh", me, 0);
	// handle to ghost value for my right neighbor
  auto gv_to_right = initial_access<double>("ghost_for_right_neigh", me, 0);

  create_work<initialize>(data, gv_to_left, gv_to_right, aux);
	// publish only the ghost points, since data remains local	
	gv_to_left.publish(n_readers=1);		
	gv_to_right.publish(n_readers=1);


	/**********************************************************
													Time loop
	**********************************************************/		
	for (int iLoop = 0; iLoop < n_iter; ++iLoop)
	{
 		auto gv_from_left_neigh 
 			= aux.is_leftmost ? read_access<double>("ghost_for_left_neigh",me,iLoop) : 
 					 read_access<double>("ghost_for_right_neigh",aux.left_neighbor,iLoop);

 		auto gv_from_right_neigh 
 			= aux.is_rightmost ? read_access<double>("ghost_for_right_neigh",me,iLoop) : 
 						read_access<double>("ghost_for_left_neigh",aux.right_neighbor,iLoop);

		gv_to_left = initial_access<double>("ghost_for_left_neigh",me,iLoop+1);
  	gv_to_right = initial_access<double>("ghost_for_right_neigh",me,iLoop+1);

  	create_work<stencil>(data, gv_from_left_neigh, gv_from_right_neigh, 
  											 gv_to_left, gv_to_right, aux);

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
  create_work<error>(data,myErr,myGlobalErr, aux);
  // will be read by all ranks except myself
	myErr.publish(n_readers=n_spmd-1);
	// each rank performs global sum: mimicing collective
	for (int iPd = 0; iPd < n_spmd; ++iPd){
		if (iPd != me){
 	 		auto iPdErr = read_access<double>("myl1error",iPd);
			create_work([=]{
				myGlobalErr.get_reference() += iPdErr.get_value();
 	 		});
		}
	}

  create_work<printError>(myGlobalErr);

  darma_finalize();
  return 0;

}//end main