

#include <darma.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;

////////////////////////////////////////////////////////////////////////////////
/* 

    An attempt at what a dot product example might look like.
    Questions to grapple with:
      1) How to express/perform collectives i.e. what should tha API be?
      2) What should the result of an all-to-all collective be?
      3) Who should publish it, and where should it be avaialable (one copy everywhere)

*/
// main() function

int main(int argc, char** argv) {

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  size_t local_vec_size = 50;

  auto vec_chunk_a = initial_access<float*>("A", me);
  auto vec_chunk_b = initial_access<float*>("B", me);

  //This is the result of local dot product.
  auto my_a_dot_b = initial_access<float>("local_AdotB", me);

  //Now for the global dot product result.
  //Should its key have anything to do with darma rank at all?
  //If not, then won't there be multiple publishes of it?
  auto a_dot_b = initial_access<float>("AdotB");

  create_work([=]{
    vec_chunk_a->allocate(local_vec_size);
    float* a = vec_chunk_a->get();
    memset(a, 1.1, local_vec_size*sizeof(float));

    vec_chunk_b->allocate(local_vec_size);
    float*b = vec_chunk_b->get();
    memset(b, 2.2, local_vec_size*sizeof(float));

    float res = 0.0;
    for(int i = 0; i<local_vec_size; i++){
      res += a[i]*b[i];
    }

    my_a_dot_b.set_value(res);
  });

  //publish the local vector chunks and result, just because.....
  vec_chunk_a.publish(); 
  vec_chunk_b.publish();
  my_a_dot_b.publish();

  //Now the call to the collective
  //first allocate memory for the result
  a_dot_b->allocate(1);
  
  all_gather(my_a_dot_b, me, n_ranks, collective_op_sum, a_dot_b->get() );

  //What actions next? Should one or all publish? Or should the result be
  //local only?


  darma_finalize();

}
