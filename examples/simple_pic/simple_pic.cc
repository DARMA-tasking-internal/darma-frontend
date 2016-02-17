
#include <mock_backend.h>



using namespace darma_runtime;


////////////////////////////////////////////////////////////////////////////////
// A user data type for holding mesh data


struct Mesh {

struct MeshElement {
  size_t index;
  double x_low[3], x_hi[3];
  size_t lower[3], upper[3];

  // SPMD Parallelism: here we simply add the global MeshElement id.
  size_t global_index;
  size_t global_lower[3], global_upper[3], coords[3];
};

  size_t num_elements;
  MeshElement *elements;
  double dx[3];
protected:
  size_t nx_, ny_, nz_;

public:

  // MPI Parallelism
  // We need to know what partition this is.
  size_t index;

  // the global id for this mesh partition, and those below/above this 
  size_t lower[3], upper[3], coords[3];

  // global element ids of the corner elements in this partition
  size_t coords_low[3], coords_high[3]; 

  ~Mesh () {
  }



};

////////////////////////////////////////////////////////////////////////////////
// Some helper functions

size_t to_global ( size_t i, size_t j, size_t k ) const{
  if (i<0 ) i += nx_; if (i>= nx_) i-= nx_;
  if (j<0 ) j += ny_; if (j>= ny_) j-= ny_;
  if (k<0 ) k += nz_; if (k>= nz_) k-= nz_;
  return i+j*nx_ + k*nx_*ny_;
}

size_t to_index(size_t nx, size_t ny, , size_t nz, double x[3])  const{
  size_t i = x[0]*nx, j = x[1]*ny, k = x[2]*nz;
  return to_global(i,j,k);
}

////////////////////////////////////////////////////////////////////////////////
// A function that initializes the mesh for this darma rank

void setUpMesh(size_t nx, size_t ny, size_t nz, size_t rank, Mesh & mesh) { 
  mesh.num_elements = nx*ny*nz; mesh.nx_ = nx; mesh.ny_ = ny; mesh.nz_ = nz ; 
  mesh.elements = (MeshElement *)malloc(num_elements * sizeof(MeshElement));
  mesh.num_elements = 0;
  mesh.dx[0] = 1./nx; mesh.dx[1] = 1./ny; mesh.dx[2] = 1./nz;
  size_t i[3] = {0,0,0};
  for (i[2]=0; i[2] < nz; ++i[2])
    for (i[1]=0; i[1] < ny; ++i[1])
      for (i[0]=0; i[0] < nx; ++i[0]) {
	assert(to_global(i[0],i[1],i[2]) == mesh.num_elements);
	MeshElement &e = mesh.elements[mesh.num_elements++];
	for (size_t idim =0; idim<3; ++idim) {
	  e.x_low[idim] = i[idim]*dx[idim];
	  e.x_hi[idim] = (i[idim]+1)*dx[idim];
	}
	e.lower[0] = to_global(i[0]-1, i[1], i[2]);
	e.upper[0] = to_global(i[0]+1, i[1], i[2]);
	e.lower[1] = to_global(i[0], i[1]-1, i[2]);
	e.upper[1] = to_global(i[0], i[1]+1, i[2]);
	e.lower[2] = to_global(i[0], i[1], i[2]-1);
	e.upper[2] = to_global(i[0], i[1], i[2]+1);
      }

  // initialize the partition id
  mesh.index = rank;
}


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

};

////////////////////////////////////////////////////////////////////////////////
// A function that initializes the particle locations for this darma rank

void initializeParticles(size_t n_parts, Mesh & mesh, Particles & ps){

  ps.parts.resize(n_parts);
  for(std::vector<Particle>::iterator it = ps.parts.begin(); it != ps.parts.end(); ++it){
    it->state = ACTIVE;
    do {
      it->x[0] = c-2*c*(drand48() - 0.5);
      it->x[1] = c-2*c*(drand48() - 0.5);
      it->x[2] = c-2*c*(drand48() - 0.5);
    } while ((it->x[0]-c)*(it->x[0]-c) + (it->x[1]-c)*(it->x[1]-c)+
	     (it->x[2]-c)*(it->x[2]-c) > c*c);


    it->element = to_index(mesh.nx_, mesh.ny_, mesh.nz_, it->x);
    do {
      it->v[0] = .125-.5*(drand48() - 0.5);
      it->v[1] = .125-.5*(drand48() - 0.5);
      it->v[2] = .125-.5*(drand48() - 0.5);
    } while ((it->v[0])*(it->v[0]) + (it->v[1])*(it->v[1])+
	   (it->v[2])*(it->v[2]) > c*c);

  }

}

////////////////////////////////////////////////////////////////////////////////
// main() function

size_t main(size_t argc, char** argv)
{
  darma_init(argc, argv);

  using namespace darma_runtime::keyword_arguments_for_publication;

  size_t me = darma_spmd_rank();
  size_t n_spmd = darma_spmd_size();

  // Create a serial mesh: i.e. local ids, cyclic boundary condition on the
  // local ids.
  size_t global_elem = 60; // 21x21x21 global mesh

  // how many partitions in each direction
  size_t partitions = 3; // partitions in each direction

  // number of elements in each dim for the local mesh partition
  size_t local_elem = global_elem/partitions;

  // number of particles per each rank
  size_t n_parts = 5000;

  {  
  auto mesh_handle = initial_access<Mesh>("mesh", me);
  auto parts_handle = initial_access<Particles>("particles",me);

  create_work([=]{
      setUpMesh(local_elem, local_elem, local_elem, me, mesh_handle.get_reference());
      initializeParticles(n_parts, mesh_handle.get_reference(), parts_handle.get_reference());
    });

  //mesh_handle.publish(n_readers=1);
  }


  darma_finalize();
}

