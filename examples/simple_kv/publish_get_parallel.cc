

#include <darma.h>

using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;

////////////////////////////////////////////////////////////////////////////////
// This example is to illustrate simple transactions with the keyvalue store, 
// but in a distributed setting. We will ask each rank to publish a float to be
// read by two readers, a rank on the left and one on the right. Then we will ask
// each rank to get two floats, those published by the left and right neighbours
// and print to screen. Periodic logic for neighbours, and each rank publsihes a 
// float that uniquely determines its rank.

// main() function

int main(int argc, char** argv) {

  darma_init(argc, argv);

  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  auto float_to_pub = initial_access<float>("floatKey", me);

  create_work([=]{

    //set_value could be replaced by the more verbose
    //->allocate, followed by, ->get() = value
    float_to_pub.set_value(2692.0 + me); //a float I like

  });

  float_to_pub.publish(n_readers=2); //two guys will read this

  size_t left_nbr = (me == 0) ? n_ranks-1 : me-1 ;
  size_t right_nbr = (me == n_ranks-1) ? 0 : me+1 ;

  auto float_from_left = read_access<float>("floatKey", left_nbr);
  auto float_from_right= read_access<float>("floatKey", right_nbr);

  create_work([=]{
    std::cout << "My rank is " << me << " values from my left/right are " << float_from_left.get_value() << " " << float_from_right.get_value() << std::endl;
  });

  darma_finalize();

}
