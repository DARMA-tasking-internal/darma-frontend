

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cassert>
#include <assert.h>
#include "mpi.h"
#include <vector>

#include "../common_heat1d.h"


/* 
	Solve 1d heat equation using forward Euler in time, 2d order FD in space.
		
	PDE: 
		dT(x)/dt = alpha * d^2T(x)/dx^2

	over (0,1), with T(0)=100, T(1)=10

	Problem is setup as follows: 
	
	Full grid: 
	    o  o  o  o  o  o  o  o  o  o  o  o  o  o  o  o
		
	Distribute uniformly accross all ranks: 

    s   o  o  o  o  *
			     *  o  o  o  o  *
							 *  o  o  o  o  *
										 *  o  o  o  o  s

		   r0		   r1 			r2         r3

	s:  are not actually needed because outside of domain, 
		but exist anyway so that each local vector has same size.

	Locally, each rank owns elements: 
			
		*  o  o  o  o  *

	where inner points are:  o 
		  ghosts points are: * 
	
	Below we use following shortcut for indices of key points:
	
		*    o   o   o   o    *
		lli  li 		 ri  rri

*/


////////////////////////////////////////////////////////////////////////////////
// main() function

int main(int argc, char** argv)
{
	int myPID;
	int comm_size;
	MPI_Init ( &argc, &argv );
	MPI_Comm_rank ( MPI_COMM_WORLD, &myPID );
	MPI_Comm_size ( MPI_COMM_WORLD, &comm_size );

	// ------------------------------------------
	assert( nx % comm_size == 0 );
	const int num_points_per_rank = nx / comm_size; 
	const int num_points_per_rank_wghosts = num_points_per_rank + 2; 

	// useful constants (see above for legend )
	const int lli = 0;
	const int li  = 1;
	const int ri  = num_points_per_rank;
	const int rri = num_points_per_rank+1;

	// If I'm 0 or n_spmd-1, I am my own neighbor on the left and right
	const bool is_leftmost = myPID == 0;
	const int left_neighbor = is_leftmost ? myPID : myPID - 1;
	const bool is_rightmost = myPID == comm_size - 1;
	const int right_neighbor = myPID == is_rightmost ? myPID : myPID + 1;
	const int leftNeigh   = myPID-1;
	const int rightNeigh  = myPID+1;

	const int tagToLeft 	= 1;	// tag for sending to left 
	const int tagFromRight 	= 1;	// tag for recv from right
	const int tagToRight 	= 2;	// tag for sending to right 
	const int tagFromLeft 	= 2;	// tag for recv from left

	// ------------------------------------------

	// initial condition: set = 50 except for BC
	// temp has size = num_points_per_rank + 2 for ghost points
	std::vector<double> temp(num_points_per_rank+2, 50.0);
	if ( is_leftmost )
	{
		temp[li] = Tl; 
	}
	if ( is_rightmost )
	{
		temp[ri] = Tr; 
	}
	std::vector<double> temp_new(temp); // needed for time advancing

	// ------------------------------------------

	// time loop 
	for (int timeLoop = 1; timeLoop <= n_iter; ++timeLoop)
	{
		MPI_Request empty;
		std::vector<MPI_Request> myReqs;
		std::vector<MPI_Status>  mySts;

		// recv from right
    if ( !is_rightmost ) 
    {
			myReqs.push_back( empty );
			MPI_Irecv( &temp[rri], 1, MPI_DOUBLE, rightNeigh, 
					   	tagFromRight, MPI_COMM_WORLD, &myReqs[myReqs.size()-1] );
    }
		// recv from left 
    if ( !is_leftmost )
    {
			myReqs.push_back( empty );
			MPI_Irecv( &temp[lli], 1, MPI_DOUBLE, leftNeigh, 
						tagFromLeft, MPI_COMM_WORLD, &myReqs[myReqs.size()-1] );
    }

		// send to left
    if ( !is_leftmost )
    {
			myReqs.push_back( empty );
			MPI_Isend( &temp[li], 1, MPI_DOUBLE, leftNeigh, 
						tagToLeft, MPI_COMM_WORLD, &myReqs[myReqs.size()-1] );
    }
		// send to right
    if ( !is_rightmost )
    {
			myReqs.push_back( empty );
			MPI_Isend( &temp[ri], 1, MPI_DOUBLE, rightNeigh, 
						tagToRight, MPI_COMM_WORLD, &myReqs[myReqs.size()-1] );
    }

    mySts.resize(myReqs.size());
    MPI_Waitall( myReqs.size(), myReqs.data(), mySts.data() );


		// update field based on FD stencil
    for (int i = 1; i <= num_points_per_rank; i++ )
    {
    	double laplacian = ( temp[i+1] - 2.0*temp[i] + temp[i-1] );
			temp_new[i] = temp[i] + alphadtOvdxSq * laplacian;
    }

    // fix the domain boundary conditions 
		if ( is_leftmost )
		{
			temp_new[li] = Tl;
		}
		if ( is_rightmost )
		{
			temp_new[ri] = Tr; 
		}

		// Update time and temperature.
    for ( int i = 1; i <= num_points_per_rank; i++ )
    {
			temp[i] = temp_new[i];
    }

	}// time loop end

	// -----------------------------------------------------

	// MAYBE PRINTING STAGE

  for ( int i = 1; i <= num_points_per_rank; i++ )
  {
		std::cout << temp[i] << std::endl;
  }

	// -----------------------------------------------------

 	MPI_Finalize();
 	return 0;
}

