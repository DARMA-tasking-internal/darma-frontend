
#include <darma.h>



using namespace darma_runtime;

enum State{
  TO_X_LO = 0,
  TO_X_HI = 1,
  TO_Y_LO = 2,
  TO_Y_HI = 3,
  TO_Z_LO = 4,
  TO_Z_HI = 5,
  EXPIRED = 6,
  ACTIVE = 7
};


////////////////////////////////////////////////////////////////////////////////
// Some helper functions

int to_index(int nx, int ny, int nz, double x[3])  {
  int i = x[0]*nx, j = x[1]*ny, k = x[2]*nz;
  return (i + j*nx + k*nx*ny);//to_global(i,j,k);
}

bool to_grid( int id, int *i, int *j, int *k , 
	      const int & nx, const int & ny, const int & nz)  {
    // NOTE: what to do if someone ask for an id that doesn't exist?
    //      - should there be a "NULL" value?
    //      - should we just exit gracefully with an error message?
    //      - return false and let the user figure out what to do?
    //
    //      Choose the third option for now.

  if( id < 0 || id >= nx*ny*nz ) {
    *i = *j = *k = -1;
    return false;
  }
    div_t result_k;
    div_t result_j;

    result_k = div ( id, (nx*ny) );
    *k = result_k.quot;

    result_j = div ( result_k.rem, nx );
    *j = result_j.quot;
    *i = result_j.rem;

    return true;
}

int to_id( int i, int j, int k, 
	   const int & nx, const int & ny, const int & nz) {

    // Apply cyclic boundary conditions
    // to each index
    if( i < 0   ) { i = nx-1; }
    else if( i >= nx ) { i = 0;    }

    if( j < 0   ) { j = ny-1; }
    else if( j >= ny ) { j = 0;    }

    if( k < 0   ) { k = nz-1; }
    else if( k >= nz ) { k = 0;    }


    return i + ( j * nx ) + ( k * nx * ny );
  }


////////////////////////////////////////////////////////////////////////////////
// A user data type for holding mesh data


struct Mesh {

struct MeshElement {
  int index;
  double x_low[3], x_hi[3];
  int lower[3], upper[3];

  // SPMD Parallelism: here we simply add the global MeshElement id.
  int global_index;
  int global_lower[3], global_upper[3], coords[3];
};

  int num_elements;
  MeshElement *elements;
  double dx[3];
public:
  int nx_, ny_, nz_;

public:

  // MPI Parallelism
  // We need to know what partition this is.
  int index;

  // the global id for this mesh partition, and those below/above this 
  int lower[3], upper[3], coords[3];

  // global element ids of the corner elements in this partition
  int coords_low[3], coords_high[3]; 

  ~Mesh () {
  }

  // A function that initializes the mesh for this darma rank
  // This used to be the constructor of this class, but in the 
  //current spec this needs to be a separate member function
  void setUpMesh(int nx, int ny, int nz, int rank) {
    num_elements = nx*ny*nz; nx_ = nx; ny_ = ny; nz_ = nz ; 
    elements = (MeshElement *)malloc(num_elements * sizeof(MeshElement));
    num_elements = 0;
    dx[0] = 1./nx; dx[1] = 1./ny; dx[2] = 1./nz;
    int i[3] = {0,0,0};
    for (i[2]=0; i[2] < nz; ++i[2])
      for (i[1]=0; i[1] < ny; ++i[1])
	for (i[0]=0; i[0] < nx; ++i[0]) {
	  MeshElement &e = elements[num_elements++];
	  for (int idim =0; idim<3; ++idim) {
	    e.x_low[idim] = i[idim]*dx[idim];
	    e.x_hi[idim] = (i[idim]+1)*dx[idim];
	  }
	  e.lower[0] = i[0]-1 + i[1]*nx     + i[2]*nx*ny;//to_global(i[0]-1, i[1], i[2]);
	  e.upper[0] = i[0]+1 + i[1]*nx     + i[2]*nx*ny;//to_global(i[0]+1, i[1], i[2]);
	  e.lower[1] = i[0]   +(i[1]-1)*nx  + i[2]*nx*ny;//to_global(i[0], i[1]-1, i[2]);
	  e.upper[1] = i[0]   +(i[1]+1)*nx  + i[2]*nx*ny;//to_global(i[0], i[1]+1, i[2]);
	  e.lower[2] = i[0]   + i[1]*nx     +(i[2]-1)*nx*ny;//to_global(i[0], i[1], i[2]-1);
	  e.upper[2] = i[0]   + i[1]*nx     +(i[2]+1)*nx*ny;//to_global(i[0], i[1], i[2]+1);
      }

    // initialize the partition id
    index = rank;
  }


  // A function that initializes the mesh partition info for this darma rank
  //This used to be a separate class (MeshOracle) in simple_pic_mpi, but
  //I've merged it with the mesh class to minimise the number of classes to deal with
  void setUpMeshPartitions(int p_x, int p_y, int p_z, 
			   int rank) {

    //set up the cartesian grid id for this mesh rank
    to_grid(rank, &coords[0], &coords[1], &coords[2], p_x, p_y, p_z);

    //get info on low/hi neighbours in each direction
    lower[0] = to_id( coords[0]-1, coords[1],   coords[2],   p_x, p_y, p_z);
    lower[1] = to_id( coords[0],   coords[1]-1, coords[2],   p_x, p_y, p_z);
    lower[2] = to_id( coords[0],   coords[1],   coords[2]-1, p_x, p_y, p_z);

    upper[0] = to_id( coords[0]+1, coords[1],   coords[2],   p_x, p_y, p_z);
    upper[1] = to_id( coords[0],   coords[1]+1, coords[2],   p_x, p_y, p_z);
    upper[2] = to_id( coords[0],   coords[1],   coords[2]+1, p_x, p_y, p_z);

    //Now info on the elements of this rank, and those on the low/hi sides
    coords_low[0] = coords[0]*nx_;
    coords_low[1] = coords[1]*ny_;
    coords_low[2] = coords[2]*nz_;

    coords_high[0] = (coords[0]+1)*nx_;
    coords_high[1] = (coords[1]+1)*ny_;
    coords_high[2] = (coords[2]+1)*nz_;

    int glob_elem_x = p_x*nx_;
    int glob_elem_y = p_y*ny_;
    int glob_elem_z = p_z*nz_;

    int eid=0; // local element index
    for( int k=coords_low[2]; k<coords_high[2]; ++k ) {
      for( int j=coords_low[1]; j<coords_high[1]; ++j ) {
        for( int i=coords_low[0]; i<coords_high[0]; ++i ) {

          elements[eid].coords[0] = i;
          elements[eid].coords[1] = j;
          elements[eid].coords[2] = k;

          elements[eid].global_index = to_id( i, j, k, 
					      glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_lower[0] = to_id( i-1, j,   k,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_lower[1] = to_id( i,   j-1, k,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_lower[2] = to_id( i,   j,   k-1,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_upper[0] = to_id( i+1, j,   k,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_upper[1] = to_id( i,   j+1, k,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          elements[eid].global_upper[2] = to_id( i,   j,   k+1,
						 glob_elem_x, glob_elem_y, glob_elem_z);

          ++eid;
        }
      }
    }

  }

};



////////////////////////////////////////////////////////////////////////////////
// A user data type for particle data

struct Particles {

  struct Particle {
    int element;
    double x[3];
    double v[3];
    double time_left;
    State state;
  };

  int num_parts;
  int active_parts_l;
  int active_parts_g;

  int num_parts_to_recv[6];
  int num_parts_to_send[6];

  std::vector<Particle> parts;

  std::vector<Particle> parts_to_xlo, parts_to_xhi;
  std::vector<Particle> parts_to_ylo, parts_to_yhi; 
  std::vector<Particle> parts_to_zlo, parts_to_zhi;

  std::vector<Particle> parts_from_xlo, parts_from_xhi;
  std::vector<Particle> parts_from_ylo, parts_from_yhi;
  std::vector<Particle> parts_from_zlo, parts_from_zhi;

  ~Particles(){
    parts.clear();
    parts.swap(parts); //ensure that memory is freed
  }

  // A function that initializes the particle locations for this darma rank
  void initializeParticles(int n_parts, int nx, int ny, int nz){

    double c = 0.125;

    parts.resize(n_parts);
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      it->state = ACTIVE;
      do {
	it->x[0] = c-2*c*(drand48() - 0.5);
	it->x[1] = c-2*c*(drand48() - 0.5);
	it->x[2] = c-2*c*(drand48() - 0.5);
    } while ((it->x[0]-c)*(it->x[0]-c) + (it->x[1]-c)*(it->x[1]-c)+
	     (it->x[2]-c)*(it->x[2]-c) > c*c);


      it->element = to_index(nx, ny, nz, it->x);
      do {
	it->v[0] = .125-.5*(drand48() - 0.5);
	it->v[1] = .125-.5*(drand48() - 0.5);
	it->v[2] = .125-.5*(drand48() - 0.5);
      } while ((it->v[0])*(it->v[0]) + (it->v[1])*(it->v[1])+
	     (it->v[2])*(it->v[2]) > c*c);

    }

  }

  // A function that moves all the particles on this rank, to the edge of the mesh
  // of this rank

  void move_all( double dt, Mesh & mesh){
    for(std::vector<Particle>::iterator it = parts.begin(); it != parts.end(); ++it){
      it->time_left = dt;
      if(it->state > TO_Z_HI){ //move(*it); }

	//The entire code below is the move for each particle and used to be 
	//a separate function, but I've inlined it all here.
	Particle &p = *it;

	int elem = p.element, next_elem = elem;
	double t = p.time_left;

	do {
	  for (int idim =0; idim<3; ++idim) { 
	    if ( p.v[idim] < 0 && p.x[idim] + p.time_left*p.v[idim] < mesh.elements[elem].x_low[idim]) {
	      double local_t = -(p.x[idim]-mesh.elements[elem].x_low[idim])/(p.time_left*p.v[idim])*p.time_left;
	      if (local_t < t) {
		t = local_t;
		next_elem = mesh.elements[elem].lower[idim];
	      }
	    }
	    if (  p.v[idim] > 0 && p.x[idim] + p.time_left*p.v[idim] > mesh.elements[elem].x_hi[idim]) {
	      double local_t = (mesh.elements[elem].x_hi[idim]-p.x[idim])/(p.time_left*p.v[idim])*p.time_left;
	      if (local_t < t) {
		t = local_t;
		next_elem = mesh.elements[elem].upper[idim];
	      }
	    }
	  }


	  if (t != p.time_left )
	    t *= (1. + 2e-16); // stupid roundoff fix
	  for (int idim=0; idim<3; ++idim) { 
	    p.x[idim] += t*p.v[idim];
	    //replace periodicity with check for boundary reached
	    if (p.x[idim] <= 0.0){
	      p.state = (State)(2*idim); //low boundary
	      break;
	    } else if (p.x[idim] >= 1.0 ){
	      p.state = (State)(2*idim + 1); //hi boundary
	      break;
	    }
	  }
	  if ( t < p.time_left){
	    p.element = next_elem;
	  }else{
	    p.state = EXPIRED;
	  }
	  p.time_left -= t; t = p.time_left;
	  elem = next_elem;

	} while (t > 0 && p.state == ACTIVE);

      }//end of if loop
    }//end of for loop

  }


};

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// main() function

int darma_main(int argc, char** argv)
{
  darma_init(argc, argv);

  using namespace darma_runtime::keyword_arguments_for_publication;

  int me = darma_spmd_rank();
  int n_spmd = darma_spmd_size();

  // Create a serial mesh: i.e. local ids, cyclic boundary condition on the
  // local ids.
  int global_elem = 60; // 21x21x21 global mesh

  // how many partitions in each direction
  int partitions_x, partitions_y, partitions_z; 
  partitions_x = partitions_y = partitions_z = 2; // partitions in each direction

  assert(n_spmd ==  partitions_x*partitions_y*partitions_z);

  // number of elements in each dim for the local mesh partition
  int local_elem = global_elem/partitions_x;

  // number of particles per each rank
  int n_parts = 5000;

    
  auto mesh_handle = initial_access<Mesh>("mesh", me);
  auto parts_handle = initial_access<Particles>("particles",me);


  //First task to initialise the meash and particles (position, velocity)
  //for this rank
  create_work([=]{

      Mesh & m = mesh_handle.get_reference(); 
      Particles & ps = parts_handle.get_reference();

      //The function below initialises some of the data members 
      //of this mesh object, but not all
      m.setUpMesh(local_elem, local_elem, local_elem, me);

      //This function initialises the remaining data members
      //Can we assume provenance of the data members that have been
      //initialised by the previous member function??
      m.setUpMeshPartitions(partitions_x, partitions_y, partitions_z, me);

      //Now initialize particles. Will m.nx_, m.ny_, m.nz_ be sensible??
      ps.initializeParticles(n_parts, m.nx_, m.ny_, m.nz_);
    });


  //now follow on task that moves that particles of this rank to the edge of the mesh
  create_work([=]{

      Mesh & m = mesh_handle.get_reference();
      Particles & ps = parts_handle.get_reference();

      double dt = 0.025;
      ps.move_all(dt, m);

    });


  mesh_handle.publish(n_readers=1);
  parts_handle.publish(n_readers=1);

  /*
  //We will be reading the mesh data, but overwriting Particles data.
  //(Q*) Can I reuse the existing handles (how)??
  auto mesh_r_handle = read_access<Mesh>("mesh", me);
  auto parts_rw_handle = read_write<Particles>("particles", me);
  */


  darma_finalize();
  return 0;
}

