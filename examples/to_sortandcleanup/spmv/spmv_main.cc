#include <mock_backend.h>
#include 


using namespace darma_runtime;

template <typename T>
struct DataArray
{
  public:

    typedef abstract::frontend::SerializationManager::zero_copy_slot_t zero_copy_slot_t;

    // Some user methods
    void allocate(size_t n_data) {
      assert(data_ == nullptr);
      if(n_data) {
        data_ = (double*)malloc(n_data * sizeof(T));
        data_size_ = n_data;
      }
    }

    T* get() { return data_; }
    const T* get() const { return data_; }

    size_t size() const { return data_size_; }

    ~DataArray() {
      if(data_size_) free(data_);
    }

  protected:

    void*& get_zero_copy_slot(zero_copy_slot_t slot) {
      return data_;
    }

    size_t zero_copy_slot_size(zero_copy_slot_t slot) const {
      return data_size_;
    }

  private:

    T* data_ = nullptr;

    size_t data_size_ = 0;

};

int main( int argc, char **argv ) 
{
  darma_init(argc,argv);
  using namespace darma_runtime::keyword_arguments_for_publication;

  size_t me = darma_spmd_rank();
  size_t num_spmd = darma_spmd_size();

  // Just apply simple row-block data partition
  size_t num_local_rows = num_global_rows / num_spmd; 
  if( me < num_global_rows & num_spmd ) ++num_local_rows;

  // Total amound of data that will be passed 
  // to do_spmv 


  // Set up local matrix data
  auto data = inital_access<data_t>("data",me);
  // Set up the ghost data that can support any numbers of neighbours
  auto ghosts[] = new ghost(num_of_ghosts); 
  for( int i = 0;i < num_of_ghosts; i++ ) {
     ghosts[i] = initeal_access_data<ghost_t>("ghost",me,i);
  }

  // Task that computes outgoing ghost
  create_work([=]{
    data->allocate();
    // Check the indices that can go to the other tasks
    for (int i = 0 ; i < n_local_nonz; i++ ) {
      if( col_ind[i] < row_begin || col_ind[i] >= row_end ) {
        //
        
      }
    }
    // Send indcices for outgoing ghost points
    for( int i = 0; i < num_neighbours; ++i ) {
       publish(my_id, dest_id[i], out_ghost_size[i] );
       publish(my_id, dest_id[i], indices[i], out_ghost_size[i] );
    }
  });

  // Need something equivalent to MPI_Allgather

  
  // Task that computes incoming ghost zones
  // They are not the same as outgoing ghost zones 
  // if the matrix is unsymmetric
  create_work([=]{
    data->allocate();
    // Check the indices that can go to the other tasks
    for (int i = 0 ; i < n_local_nonz; i++ ) {
      if( col_ind[i] < row_begin || col_ind[i] >= row_end ) {
        //
        
      }
    }

    //
    // Send indcices for outgoing ghost points
    //
    for( int i = 0; i < num_neighbours; ++i ) {
       publish(my_id, dest_id[i], out_ghost_size[i] );
       publish(my_id, dest_id[i], indices[i], out_ghost_size[i] );
    }
  });
 
 
  // Task that create
  create_work([=]{
  });

  // Perform Single SpMV
  create_work([=]{


  });

  
}  
