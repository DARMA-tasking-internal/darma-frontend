#include <darma.h>
using namespace darma_runtime;
using namespace darma_runtime::keyword_arguments_for_publication;
int darma_main(int argc, char** argv) 
{
  darma_init(argc, argv);
  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  // define neighbors with periodic arrangement
  size_t left_nbr = (me == 0) ? n_ranks-1 : me-1 ;
  size_t right_nbr = (me == n_ranks-1) ? 0 : me+1 ;

  auto float_to_pub = initial_access<float>("floatKey", me);
  create_work([=]
  {
    //set_value could be replaced by the more verbose
    //->allocate, followed by, ->get() = value
    float_to_pub.set_value(2692.0 + me); //a float I like
  });

  float_to_pub.publish(n_readers=2); 
  //n_readers=2: two read_access handles will be defined for this

  // fetch the data
  auto float_from_left = read_access<float>("floatKey", left_nbr);
  auto float_from_right= read_access<float>("floatKey", right_nbr);
  create_work([=]
  {
    std::cout << "My rank is " << me 
              << " values from my left/right are " 
              << float_from_left.get_value() << " " 
              << float_from_right.get_value() << std::endl;
  });

  darma_finalize();
  return 0;
}
