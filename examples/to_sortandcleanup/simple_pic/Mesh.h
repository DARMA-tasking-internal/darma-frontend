#ifndef MESH_H_
#define MESH_H_

#include <cassert>

//
// This does basic integer computations for a regular cartesian grid in 3D.
// to_grid(...) maps an id to it's corresponding i,j,k coordinates.
// to_id(...) maps an {i,j,k} triplet to it's corresponding id.
// size(...) returns the total number of elements
//
// Grid3D can be used to compute topologies for:
//  - the global elements
//  - the local elements of a mesh parition
//  _ the global view of patches if they are arranged in a  regular cartesian
//  grid.

class Grid3D {
public:
  int nx, ny, nz;
  int num_elements;

  Grid3D( int x, int y, int z ) : nx(x), ny(y), nz(z), num_elements(nx * ny * nz) {}

  bool to_grid( int id, int *i, int *j, int *k ) const {
    // NOTE: what to do if someone ask for an id that doesn't exist?
    //      - should there be a "NULL" value?
    //      - should we just exit gracefully with an error message?
    //      - return false and let the user figure out what to do?
    //
    //      Choose the third option for now.

    if( id < 0 || id >= num_elements ) {
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

  int to_id( int i, int j, int k ) const {

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

  int size() { return nx * ny * nz; }

};



struct MeshElement {
  int index;
  double x_low[3], x_hi[3];
  int lower[3], upper[3];

  // MPI Parallelism: here we simply add the global MeshElement id.
  int global_index;
  int global_lower[3], global_upper[3], coords[3];
};

struct Mesh {
  int num_elements;
  MeshElement *elements;
  double dx[3];
protected:
  int nx_, ny_, nz_;
  int to_global ( int i, int j, int k ) const{
    if (i<0 ) i += nx_; if (i>= nx_) i-= nx_;
    if (j<0 ) j += ny_; if (j>= ny_) j-= ny_;
    if (k<0 ) k += nz_; if (k>= nz_) k-= nz_;
    return i+j*nx_ + k*nx_*ny_;
  }
public:

  // MPI Parallelism
  // We need to know what partition this is.
  int index;

  // the global id for this mesh partition, and those below/above this 
  int lower[3], upper[3], coords[3];

  // global element ids of the corner elements in this partition
  int coords_low[3], coords_high[3]; 

  Mesh(int nx, int ny, int nz) : num_elements(nx*ny*nz), nx_(nx), ny_(ny), nz_(nz) {
    elements = (MeshElement *)malloc(num_elements * sizeof(MeshElement));
    num_elements = 0;
    dx[0] = 1./nx; dx[1] = 1./ny; dx[2] = 1./nz;
    int i[3] = {0,0,0};
    for (i[2]=0; i[2] < nz; ++i[2])
      for (i[1]=0; i[1] < ny; ++i[1])
        for (i[0]=0; i[0] < nx; ++i[0]) {
          assert(to_global(i[0],i[1],i[2]) == num_elements);
          MeshElement &e = elements[num_elements++];
          for (int idim =0; idim<3; ++idim) {
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
    index = 0;
  }

  ~Mesh () {
    free(elements);
  }

  int to_index(double x[3])  const{
    int i = x[0]*nx_, j = x[1]*ny_, k = x[2]*nz_;
    return to_global(i,j,k);
  }

};


struct element_coords {
  int index;
  int i,j,k;
};


//
// MeshOracle has knowledge about;
//  * Element adjacencies in the global mesh
//  * Partition adjacendies in the global mesh
//  The above information is computed and not stored.
//  Elements are numbered sequentially increasing first in X, then Y, and
//  finally Z. This feature is used to map the partition ID to the global ids of
//  each of it's elements.
//
//  NOTE: don't change the global element or partition numbering. This is
//  micro(simple)-pic so we want things as simple as possible.
class MeshOracle {
public:

  Grid3D mesh_grid;
  Grid3D partition_grid;
  int num_partition_elem_x, num_partition_elem_y, num_partition_elem_z;

  // grid ids of neighboring partitions
  int part_lower, part_upper;

  MeshOracle( int nex_, int ney_, int nez_,  // number of global elements in x,y,z
      int npx_, int npy_, int npz_,  // number of partitions in x,y,z
      int nlx_, int nly_, int nlz_ ) // number of local elements for this partition in x,y,z
  : mesh_grid( nex_, ney_, nez_ ),
    partition_grid( npx_, npy_, npz_ ),
    num_partition_elem_x(nlx_), num_partition_elem_y(nly_), num_partition_elem_z(nlz_)
  {
  }

  void add_partition_data( int partition_id, Mesh * mesh ) {
    // add global partition index: mesh->index
    mesh->index = partition_id;

    // add global grid coords
    partition_grid.to_grid( mesh->index, &mesh->coords[0], &mesh->coords[1], &mesh->coords[2] );

    // add partition neighbor information: mesh->lower, mesh->upper
    /*
    mesh->lower[0] = partition_grid.to_grid( mesh->index, &mesh->coords[0]-1, &mesh->coords[1],   &mesh->coords[2]   );
    mesh->lower[1] = partition_grid.to_grid( mesh->index, &mesh->coords[0],   &mesh->coords[1]-1, &mesh->coords[2]   );
    mesh->lower[2] = partition_grid.to_grid( mesh->index, &mesh->coords[0],   &mesh->coords[1],   &mesh->coords[2]-1 );

    mesh->upper[0] = partition_grid.to_grid( mesh->index, &mesh->coords[0]+1, &mesh->coords[1],   &mesh->coords[2]   );
    mesh->upper[1] = partition_grid.to_grid( mesh->index, &mesh->coords[0],   &mesh->coords[1]+1, &mesh->coords[2]   );
    mesh->upper[2] = partition_grid.to_grid( mesh->index, &mesh->coords[0],   &mesh->coords[1],   &mesh->coords[2]+1 );
    */

    //was this intended rather than block above?? Check with Gary
    mesh->lower[0] = partition_grid.to_id( mesh->coords[0]-1, mesh->coords[1],   mesh->coords[2]   );
    mesh->lower[1] = partition_grid.to_id( mesh->coords[0],   mesh->coords[1]-1, mesh->coords[2]   );
    mesh->lower[2] = partition_grid.to_id( mesh->coords[0],   mesh->coords[1],   mesh->coords[2]-1 );

    mesh->upper[0] = partition_grid.to_id( mesh->coords[0]+1, mesh->coords[1],   mesh->coords[2]   );
    mesh->upper[1] = partition_grid.to_id( mesh->coords[0],   mesh->coords[1]+1, mesh->coords[2]   );
    mesh->upper[2] = partition_grid.to_id( mesh->coords[0],   mesh->coords[1],   mesh->coords[2]+1 );


    // add global information for each element; particle.global_lower,
    // partition_coords_to_element_coords;

    mesh->coords_low[0]  =  mesh->coords[0]      * num_partition_elem_x;
    mesh->coords_low[1]  =  mesh->coords[1]      * num_partition_elem_y;
    mesh->coords_low[2]  =  mesh->coords[2]      * num_partition_elem_z;

    mesh->coords_high[0] = (mesh->coords[0] + 1) * num_partition_elem_x;
    mesh->coords_high[1] = (mesh->coords[1] + 1) * num_partition_elem_y;
    mesh->coords_high[2] = (mesh->coords[2] + 1) * num_partition_elem_z;

    int eid=0; // local element index
    for( int k=mesh->coords_low[2]; k<mesh->coords_high[2]; ++k ) {
      for( int j=mesh->coords_low[1]; j<mesh->coords_high[1]; ++j ) {
        for( int i=mesh->coords_low[0]; i<mesh->coords_high[0]; ++i ) {

          mesh->elements[eid].coords[0] = i;
          mesh->elements[eid].coords[1] = j;
          mesh->elements[eid].coords[2] = k;

          mesh->elements[eid].global_index = mesh_grid.to_id( i, j, k );

          mesh->elements[eid].global_lower[0] = mesh_grid.to_id( i-1, j,   k  );
          mesh->elements[eid].global_lower[1] = mesh_grid.to_id( i,   j-1, k  );
          mesh->elements[eid].global_lower[2] = mesh_grid.to_id( i,   j,   k-1);

          mesh->elements[eid].global_upper[0] = mesh_grid.to_id( i+1, j,   k  );
          mesh->elements[eid].global_upper[1] = mesh_grid.to_id( i,   j+1, k  );
          mesh->elements[eid].global_upper[2] = mesh_grid.to_id( i,   j,   k+1);

          ++eid;
        }
      }
    }
  }

};



#endif
